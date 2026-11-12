#include "klib/log/log.hpp"
#include "klib/core/string/c_string.hpp"
#include "klib/core/string/lerp_expr/scanner.hpp"
#include "klib/core/visitor.hpp"
#include "klib/log/file.hpp"
#include <chrono>
#include <condition_variable>
#include <fstream>
#include <mutex>
#include <print>
#include <thread>

namespace klib {
namespace log {
namespace {
[[maybe_unused]] constexpr auto to_filename(std::string_view path) -> std::string_view {
	auto const i = path.find_last_of("\\/");
	if (i == std::string_view::npos) {
		return path;
	}
	return path.substr(i + 1);
}

struct FileImpl {
	auto start(std::string path_) -> bool {
		stop();
		auto file = std::ofstream{path_};
		if (!file.is_open()) {
			return false;
		}
		path = std::move(path_);
		m_thread = std::jthread{[this](std::stop_token const& s) {
			thunk(s);
		}};
		return true;
	}

	void stop() {
		if (!is_running()) {
			return;
		}
		m_thread.request_stop();
		m_thread.join();
		path.clear();
		m_queue.clear();
	}

	[[nodiscard]] auto is_running() const -> bool { return m_thread.joinable(); }

	void print(CString const line) {
		if (!is_running()) {
			return;
		}
		auto lock = std::unique_lock{m_mutex};
		m_queue.emplace_back(line.as_view());
		lock.unlock();
		m_cv.notify_one();
	}

	std::string path{};

  private:
	void thunk(std::stop_token const& s) {
		while (!s.stop_requested()) {
			auto lock = std::unique_lock{m_mutex};
			m_cv.wait(lock, s, [this] {
				return !m_queue.empty();
			});
			auto queue = std::move(m_queue);
			lock.unlock();
			auto file = std::ofstream{path, std::ios::app};
			for (auto const& line : queue) {
				file << line;
			}
		}
	}

	std::mutex m_mutex{};
	std::condition_variable_any m_cv{};
	std::vector<std::string> m_queue{};
	std::jthread m_thread{};
};

enum class Identifier : std::int8_t { None, Level, Tag, ThreadId, Message, Timestamp, FileName, LineNumber };

class Formatter {
  public:
	void set_interpolate_format(std::string expression) {
		m_atoms.clear();
		m_expression = std::move(expression);
		auto const per_token = [&](Token const& token) {
			switch (token.type) {
			case Token::Type::Identifier: m_atoms.emplace_back(to_identifier(token.lexeme)); break;
			case Token::Type::String: m_atoms.emplace_back(token.lexeme); break;
			default: break;
			}
		};
		lerp_expr::tokenize(m_expression, per_token);
	}

	[[nodiscard]] auto format(Input const& input) const -> std::string {
		static constexpr auto reserve_v{128uz};
		auto ret = std::string{};
		ret.reserve(input.message.size() + reserve_v);
		auto const visitor = Visitor{
			[&](std::string_view const s) {
				ret.append(s);
			},
			[&](Identifier const i) {
				format_identifier(ret, input, i);
			},
		};
		for (auto const& atom : m_atoms) {
			std::visit(visitor, atom);
		}
		return ret;
	}

  private:
	using Token = lerp_expr::Token;
	using Atom = std::variant<std::string_view, Identifier>;

	[[nodiscard]] static constexpr auto to_identifier(std::string_view const word) -> Identifier {
		if (word == "level") {
			return Identifier::Level;
		}
		if (word == "tag") {
			return Identifier::Tag;
		}
		if (word == "thread_id") {
			return Identifier::ThreadId;
		}
		if (word == "message") {
			return Identifier::Message;
		}
		if (word == "timestamp") {
			return Identifier::Timestamp;
		}
		if (word == "file_name") {
			return Identifier::FileName;
		}
		if (word == "line_number") {
			return Identifier::LineNumber;
		}
		return Identifier::None;
	}

	static void format_identifier(std::string& out, Input const& input, Identifier const identifier) {
		using namespace std::chrono;
		switch (identifier) {
		case Identifier::Level: out += *level_char_map.to_value(input.level); break;
		case Identifier::Tag: out.append(input.tag); break;
		case Identifier::ThreadId: {
			auto const thread_id = std::to_underlying(get_thread_id());
			std::format_to(std::back_inserter(out), "{:02}", thread_id);
			break;
		}
		case Identifier::Message: out.append(input.message); break;
		case Identifier::Timestamp: {
			auto const timestamp = zoned_time{current_zone(), time_point_cast<seconds>(system_clock::now())};
			std::format_to(std::back_inserter(out), "{:%T}", timestamp);
			break;
		}
		case Identifier::FileName: out.append(to_filename(input.file_name)); break;
		case Identifier::LineNumber: std::format_to(std::back_inserter(out), "{}", input.line_number); break;

		case Identifier::None:
		default: break;
		}
	}

	std::string m_expression{};
	std::vector<Atom> m_atoms{};
};

struct Storage {
	[[nodiscard]] static auto self() -> Storage& {
		static auto ret = Storage{};
		return ret;
	}

	explicit Storage() {
		auto const _ = get_thread_id();
		m_formatter.set_interpolate_format(std::string{interpolate_format_v});
	}

	auto attach_file(std::string path) -> bool {
		auto lock = std::scoped_lock{m_mutex};
		return m_file.start(std::move(path));
	}

	void detach_file() {
		auto lock = std::scoped_lock{m_mutex};
		m_file.stop();
	}

	[[nodiscard]] auto get_file_path() const -> std::string {
		auto lock = std::scoped_lock{m_mutex};
		return m_file.path;
	}

	void set_colors(std::optional<Colors> const& colors) {
		auto lock = std::scoped_lock{m_mutex};
		m_colors = colors;
	}

	[[nodiscard]] auto get_colors() const -> std::optional<Colors> {
		auto lock = std::scoped_lock{m_mutex};
		return m_colors;
	}

	void set_interpolate_format(std::string interpolate_format) {
		auto lock = std::scoped_lock{m_mutex};
		m_formatter.set_interpolate_format(std::move(interpolate_format));
	}

	[[nodiscard]] auto format(Input const& input) const -> std::string {
		auto lock = std::scoped_lock{m_mutex};
		return m_formatter.format(input);
	}

	void print_to_file(CString const line) {
		auto lock = std::scoped_lock{m_mutex};
		m_file.print(line);
	}

	std::atomic<Level> max_level{Level::Debug};

  private:
	mutable std::mutex m_mutex{};
	FileImpl m_file{};
	std::optional<Colors> m_colors{level_color_map};
	Formatter m_formatter{};
};
} // namespace

File::File(std::string path) : m_path(std::move(path)) {
	if (m_path.empty()) {
		return;
	}
	Storage::self().attach_file(m_path);
}

File::~File() { Storage::self().detach_file(); }

auto File::is_attached() const -> bool { return m_path == Storage::self().get_file_path(); }
} // namespace log

void log::set_max_level(Level level) { Storage::self().max_level = level; }
auto log::get_max_level() -> Level { return Storage::self().max_level; }

void log::set_colors(std::optional<Colors> const& colors) { Storage::self().set_colors(colors); }
auto log::get_colors() -> std::optional<Colors> { return Storage::self().get_colors(); }

void log::set_interpolate_format(std::string interpolate_format) {
	Storage::self().set_interpolate_format(std::move(interpolate_format));
}

auto log::get_thread_id() -> ThreadId {
	static auto s_id = std::atomic<std::underlying_type_t<ThreadId>>{};
	thread_local auto const ret = s_id++;
	return ThreadId{ret};
}

auto log::format(Input const& input) -> std::string {
	auto ret = Storage::self().format(input);
	ret.append("\n");
	return ret;
}

void log::print(Input const& input) {
	if (input.level > Storage::self().max_level) {
		return;
	}

	auto const text = format(input);

	auto* out = input.level == Level::Error ? stderr : stdout;
	auto const do_print = [&](std::optional<escape::Rgb> const rgb) {
		if (!rgb) {
			std::print(out, "{}", text);
			return;
		}
		auto const fg = escape::foreground(*rgb);
		std::print(out, "{}{}{}", fg, text, escape::clear);
	};

	if (auto const colors = Storage::self().get_colors()) {
		do_print(*colors->to_value(input.level));
	} else {
		do_print({});
	}
	std::fflush(out);

#ifdef _WIN32
	OutputDebugStringA(text.c_str());
#endif

	Storage::self().print_to_file(text.c_str());
}
} // namespace klib
