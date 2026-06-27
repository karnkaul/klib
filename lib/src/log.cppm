export module klib.log;

export import std;
export import klib.core;
export import klib.string;

import klib.lerp_expr;

export namespace klib {
namespace log {
enum class Level : std::int8_t { Error, Warn, Info, Debug, COUNT_ };
inline constexpr auto debug_enabled_v = debug_v;

using Colors = EnumMap<Level, std::optional<escape::Rgb>>;
auto const level_color_map = Colors{
	{Level::Error, escape::Rgb{241, 76, 76}},
	{Level::Warn, escape::Rgb{245, 245, 67}},
	{Level::Info, escape::Rgb{229, 229, 229}},
	{Level::Debug, escape::Rgb{170, 170, 170}},
};

template <typename... Args>
struct BasicFmt : std::basic_format_string<char, Args...> {
	template <std::convertible_to<std::string_view> T>
	consteval BasicFmt(T const& t, std::source_location const& sloc = std::source_location::current())
		: std::basic_format_string<char, Args...>(t), sloc(sloc) {}

	std::source_location sloc;
};

template <typename... Args>
using Fmt = BasicFmt<std::type_identity_t<Args>...>;

template <typename... Args>
void print(Level level, std::string_view tag, Fmt<Args...> const& fmt, Args&&... args);

template <typename... Args>
void error(std::string_view tag, Fmt<Args...> const& fmt, Args&&... args) {
	print(Level::Error, tag, fmt, std::forward<Args>(args)...);
}

template <typename... Args>
void warn(std::string_view tag, Fmt<Args...> const& fmt, Args&&... args) {
	print(Level::Warn, tag, fmt, std::forward<Args>(args)...);
}

template <typename... Args>
void info(std::string_view tag, Fmt<Args...> const& fmt, Args&&... args) {
	print(Level::Info, tag, fmt, std::forward<Args>(args)...);
}

template <typename... Args>
void debug(std::string_view tag, Fmt<Args...> const& fmt, Args&&... args) {
	if constexpr (!debug_enabled_v) { return; }
	print(Level::Debug, tag, fmt, std::forward<Args>(args)...);
}

// NOLINTNEXTLINE(performance-enum-size)
enum struct ThreadId : std::int64_t { Main = 0 };

struct Input {
	Level level{};
	std::string_view tag{};
	std::string_view message{};
	std::string_view file_name{};
	std::uint64_t line_number{};
};

constexpr auto debug_interpolate_format_v = std::string_view{"[{level}] [{tag}/{thread_id}] {message} [{timestamp}] [{file_name}:{line_number}]"};
constexpr auto ndebug_interpolate_format_v = std::string_view{"[{level}] [{tag}/{thread_id}] {message} [{timestamp}]"};
constexpr auto interpolate_format_v = debug_v ? debug_interpolate_format_v : ndebug_interpolate_format_v;

void set_max_level(Level level);
[[nodiscard]] auto get_max_level() -> Level;

void set_colors(std::optional<Colors> const& colors);
[[nodiscard]] auto get_colors() -> std::optional<Colors>;

void set_interpolate_format(std::string interpolate_format);

[[nodiscard]] auto get_thread_id() -> ThreadId;

[[nodiscard]] auto format(Input const& input) -> std::string;
void print(Input const& input);

class Tagged {
  public:
	explicit Tagged(std::string_view const tag, Level const max_level = Level::Debug) : max_level(max_level), m_tag(tag) {}

	template <typename... Args>
	void error(Fmt<Args...> const& fmt, Args&&... args) const {
		log::error(m_tag, fmt, std::forward<Args>(args)...);
	}

	template <typename... Args>
	void warn(Fmt<Args...> const& fmt, Args&&... args) const {
		if (max_level < Level::Warn) { return; }
		log::warn(m_tag, fmt, std::forward<Args>(args)...);
	}

	template <typename... Args>
	void info(Fmt<Args...> const& fmt, Args&&... args) const {
		if (max_level < Level::Info) { return; }
		log::info(m_tag, fmt, std::forward<Args>(args)...);
	}

	template <typename... Args>
	void debug(Fmt<Args...> const& fmt, Args&&... args) const {
		if constexpr (!debug_enabled_v) { return; }
		if (max_level < Level::Debug) { return; }
		log::debug(m_tag, fmt, std::forward<Args>(args)...);
	}

	Level max_level{Level::Debug};

  private:
	std::string_view m_tag{};
};

template <typename Type>
class Typed : public Tagged {
  public:
	explicit Typed(Level max_level = Level::Debug) : Tagged(demangled_name<Type>(), max_level) {}
};

class File {
  public:
	File(File const&) = delete;
	File(File&&) = delete;
	auto operator=(File const&) = delete;
	auto operator=(File&&) = delete;

	explicit File(std::string path = "debug.log");
	~File();

	[[nodiscard]] auto is_attached() const -> bool;
	[[nodiscard]] auto get_path() const -> std::string_view { return m_path; }

  private:
	std::string m_path;
};
} // namespace log

template <typename... Args>
void log::print(Level const level, std::string_view const tag, Fmt<Args...> const& fmt, Args&&... args) {
	if (level > get_max_level()) { return; }
	auto const message = std::format(fmt, std::forward<Args>(args)...);
	auto const input = Input{
		.level = level,
		.tag = tag,
		.message = message,
		.file_name = fmt.sloc.file_name(),
		.line_number = fmt.sloc.line(),
	};
	print(input);
}
} // namespace klib

namespace klib {
namespace log {
namespace {
[[maybe_unused]] constexpr auto to_filename(std::string_view path) -> std::string_view {
	auto const i = path.find_last_of("\\/");
	if (i == std::string_view::npos) { return path; }
	return path.substr(i + 1);
}

[[nodiscard]] constexpr auto to_char(Level const level) {
	switch (level) {
	case Level::Error: return 'E';
	case Level::Warn: return 'W';
	case Level::Info: return 'I';
	case Level::Debug: return 'D';
	default: return '?';
	}
}

struct FileImpl {
	auto start(std::string path_) -> bool {
		stop();
		auto file = std::ofstream{path_};
		if (!file.is_open()) { return false; }
		path = std::move(path_);
		m_thread = std::jthread{[this](std::stop_token const& s) { thunk(s); }};
		return true;
	}

	void stop() {
		if (!is_running()) { return; }
		m_thread.request_stop();
		m_thread.join();
		path.clear();
		m_queue.clear();
	}

	[[nodiscard]] auto is_running() const -> bool { return m_thread.joinable(); }

	void print(CString const line) {
		if (!is_running()) { return; }
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
			m_cv.wait(lock, s, [this] { return !m_queue.empty(); });
			auto queue = std::move(m_queue);
			lock.unlock();
			auto file = std::ofstream{path, std::ios::app};
			for (auto const& line : queue) { file << line; }
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
			[&](std::string_view const s) { ret.append(s); },
			[&](Identifier const i) { format_identifier(ret, input, i); },
		};
		for (auto const& atom : m_atoms) { std::visit(visitor, atom); }
		return ret;
	}

  private:
	using Token = lerp_expr::Token;
	using Atom = std::variant<std::string_view, Identifier>;

	[[nodiscard]] static constexpr auto to_identifier(std::string_view const word) -> Identifier {
		if (word == "level") { return Identifier::Level; }
		if (word == "tag") { return Identifier::Tag; }
		if (word == "thread_id") { return Identifier::ThreadId; }
		if (word == "message") { return Identifier::Message; }
		if (word == "timestamp") { return Identifier::Timestamp; }
		if (word == "file_name") { return Identifier::FileName; }
		if (word == "line_number") { return Identifier::LineNumber; }
		return Identifier::None;
	}

	static void format_identifier(std::string& out, Input const& input, Identifier const identifier) {
		switch (identifier) {
		case Identifier::Level: out += to_char(input.level); break;
		case Identifier::Tag: out.append(input.tag); break;
		case Identifier::ThreadId: {
			auto const thread_id = std::to_underlying(get_thread_id());
			std::format_to(std::back_inserter(out), "{:02}", thread_id);
			break;
		}
		case Identifier::Message: out.append(input.message); break;
		case Identifier::Timestamp: {
			using namespace std::chrono;
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

auto g_storage = Storage{}; // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)
} // namespace

File::File(std::string path) : m_path(std::move(path)) {
	if (m_path.empty()) { return; }
	g_storage.attach_file(m_path);
}

File::~File() { g_storage.detach_file(); }

auto File::is_attached() const -> bool { return m_path == g_storage.get_file_path(); }
} // namespace log

void log::set_max_level(Level level) { g_storage.max_level = level; }
auto log::get_max_level() -> Level { return g_storage.max_level; }

void log::set_colors(std::optional<Colors> const& colors) { g_storage.set_colors(colors); }
auto log::get_colors() -> std::optional<Colors> { return g_storage.get_colors(); }

void log::set_interpolate_format(std::string interpolate_format) { g_storage.set_interpolate_format(std::move(interpolate_format)); }

auto log::get_thread_id() -> ThreadId {
	static auto s_id = std::atomic<std::underlying_type_t<ThreadId>>{};
	thread_local auto const ret = s_id++;
	return ThreadId{ret};
}

auto log::format(Input const& input) -> std::string {
	auto ret = g_storage.format(input);
	ret.append("\n");
	return ret;
}

void log::print(Input const& input) {
	if (input.level > g_storage.max_level) { return; }

	auto const text = format(input);

	auto& out = input.level == Level::Error ? std::cerr : std::cout;
	auto const do_print = [&](std::optional<escape::Rgb> const rgb) {
		if (!rgb) {
			std::print(out, "{}", text);
			return;
		}
		auto const fg = escape::foreground(*rgb);
		std::print(out, "{}{}{}", fg, text, escape::clear);
	};

	if (auto const colors = g_storage.get_colors()) {
		do_print(*colors->to_value(input.level));
	} else {
		do_print({});
	}
	out.flush();

#if defined(_WIN32)
	OutputDebugStringA(text.c_str());
#endif

	g_storage.print_to_file(text.c_str());
}
} // namespace klib
