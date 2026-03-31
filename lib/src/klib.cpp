#include "klib/debug/assert.hpp"
#include <algorithm>
#include <array>
#include <atomic>
#include <cassert>
#include <chrono>
#include <condition_variable>
#include <cstddef>
#include <deque>
#include <filesystem>
#include <fstream>
#include <mutex>
#include <numeric>
#include <print>
#include <ranges>
#include <string_view>
#include <thread>
#include <utility>
#include <vector>

#if defined(_WIN32)
#if !defined(WIN32_LEAN_AND_MEAN)
#define WIN32_LEAN_AND_MEAN
#endif
#if !defined(NOMINMAX)
#define NOMINMAX
#endif
#if !defined(UNICODE)
#define UNICODE
#endif
#include <Windows.h>
#endif

namespace chr = std::chrono;

// version

#include "klib/string/from_chars.hpp"
#include "klib/version.hpp"

auto std::formatter<klib::Version>::format(klib::Version const& version, format_context& fc) -> format_context::iterator {
	return format_to(fc.out(), "v{}.{}.{}", version.major, version.minor, version.patch);
}

auto klib::to_version(std::string_view text) -> Version {
	if (text.starts_with('v')) { text.remove_prefix(1); }
	auto fc = FromChars{.text = text};
	auto ret = Version{};
	if (!fc(ret.major) || !fc.advance_if('.') || !fc(ret.minor) || !fc.advance_if('.') || !fc(ret.patch)) { return {}; }
	return ret;
}

// task

#include "klib/task/queue.hpp"

namespace klib {
namespace task {
void Task::do_execute() {
	m_status = Status::Executing;
	try {
		execute();
	} catch (...) {}
	finalize();
}

void Task::do_drop() {
	m_status = Task::Status::Dropped;
	finalize();
}

void Task::finalize() {
	switch (m_status) {
	case Status::Executing: m_status = Status::Completed; break;
	default: break;
	}
	m_busy = false;
	m_busy.notify_all();
}

struct Queue::Impl {
	Impl(Impl const&) = delete;
	Impl(Impl&&) = delete;
	auto operator=(Impl const&) = delete;
	auto operator=(Impl&&) = delete;

	explicit Impl(CreateInfo const& create_info) : m_create_info(create_info) { create_workers(); }

	~Impl() {
		drop_enqueued();
		destroy_workers();
	}

	[[nodiscard]] auto thread_count() const -> ThreadCount { return m_create_info.thread_count; }

	[[nodiscard]] auto enqueued_count() const -> std::size_t {
		auto lock = std::scoped_lock{m_mutex};
		return m_queue.size();
	}

	[[nodiscard]] auto can_enqueue(std::size_t const count) const -> bool {
		if (m_draining) { return false; }
		if (m_create_info.max_elements == ElementCount::Unbounded) { return true; }
		return enqueued_count() + count < std::size_t(m_create_info.max_elements);
	}

	auto enqueue(std::span<Task* const> tasks) -> bool {
		if (tasks.empty()) { return true; }
		if (!can_enqueue(tasks.size())) { return false; }
		for (auto* task : tasks) {
			assert(!task->is_busy());
			if (task->m_id == Task::Id::None) { task->m_id = Task::Id{++m_prev_id}; }
			task->m_status = Task::Status::Queued;
			task->m_busy = true;
		}
		auto lock = std::unique_lock{m_mutex};
		m_queue.insert(m_queue.end(), tasks.begin(), tasks.end());
		lock.unlock();
		if (tasks.size() > 1) {
			m_work_cv.notify_all();
		} else {
			m_work_cv.notify_one();
		}
		return true;
	}

	auto fork_join(std::span<Task* const> tasks) -> Task::Status {
		if (tasks.empty()) { return Task::Status::None; }
		if (!enqueue(tasks)) { return Task::Status::Dropped; }
		auto const got_dropped = [](Task* task) {
			task->wait();
			return task->m_status == Task::Status::Dropped;
		};
		if (std::ranges::any_of(tasks, got_dropped)) { return Task::Status::Dropped; }
		return Task::Status::Completed;
	}

	void pause() { m_paused = true; }

	void resume() {
		if (!m_paused) { return; }
		m_paused = false;
		m_work_cv.notify_all();
	}

	void drain_and_wait() {
		resume();
		m_draining = true;
		auto lock = std::unique_lock{m_mutex};
		m_empty_cv.wait(lock, [this] { return m_queue.empty(); });
		assert(m_queue.empty());
		lock.unlock();
		recreate_workers();
		m_draining = false;
	}

	void drop_enqueued() {
		auto lock = std::scoped_lock{m_mutex};
		for (auto* task : m_queue) { task->do_drop(); }
		m_queue.clear();
	}

  private:
	void create_workers() {
		auto const count = std::size_t(m_create_info.thread_count);
		m_threads.reserve(count);
		while (m_threads.size() < count) {
			m_threads.emplace_back([this](std::stop_token const& s) { thunk(s); });
		}
	}

	void destroy_workers() {
		for (auto& thread : m_threads) { thread.request_stop(); }
		m_work_cv.notify_all();
		m_threads.clear();
	}

	void recreate_workers() {
		destroy_workers();
		create_workers();
	}

	void thunk(std::stop_token const& s) {
		while (!s.stop_requested()) {
			auto lock = std::unique_lock{m_mutex};
			if (!m_work_cv.wait(lock, s, [this] { return !m_paused && !m_queue.empty(); })) { return; }
			if (s.stop_requested()) { return; }
			auto* task = m_queue.front();
			m_queue.pop_front();
			auto const observed_empty = m_queue.empty();
			lock.unlock();
			task->do_execute();
			if (observed_empty) { m_empty_cv.notify_one(); }
		}
	}

	CreateInfo m_create_info{};
	mutable std::mutex m_mutex{};
	std::condition_variable_any m_work_cv{};
	std::condition_variable m_empty_cv{};
	std::atomic_bool m_paused{};
	std::atomic_bool m_draining{};

	std::deque<Task*> m_queue{};
	std::vector<std::jthread> m_threads{};

	std::underlying_type_t<Task::Id> m_prev_id{};
};

void Queue::Deleter::operator()(Impl* ptr) const noexcept { std::default_delete<Impl>{}(ptr); }

Queue::Queue(CreateInfo create_info) {
	create_info.thread_count = std::clamp(create_info.thread_count, ThreadCount::Minimum, get_max_threads());
	m_impl.reset(new Impl(create_info)); // NOLINT(cppcoreguidelines-owning-memory)
}

auto Queue::thread_count() const -> ThreadCount {
	if (!m_impl) { return ThreadCount{0}; }
	return m_impl->thread_count();
}

auto Queue::enqueued_count() const -> std::size_t {
	if (!m_impl) { return 0; }
	return m_impl->enqueued_count();
}

auto Queue::can_enqueue(std::size_t const count) const -> bool {
	if (!m_impl) { return false; }
	return m_impl->can_enqueue(count);
}

auto Queue::enqueue(Task& task) -> bool {
	auto const tasks = std::array{&task};
	return enqueue(tasks);
}

auto Queue::enqueue(std::span<Task* const> tasks) -> bool {
	if (!m_impl) { return false; }
	return m_impl->enqueue(tasks);
}

auto Queue::fork_join(std::span<Task* const> tasks) -> Task::Status {
	if (!m_impl) { return Task::Status::None; }
	return m_impl->fork_join(tasks);
}

void Queue::pause() {
	if (!m_impl) { return; }
	m_impl->pause();
}

void Queue::resume() {
	if (!m_impl) { return; }
	m_impl->resume();
}

void Queue::drain_and_wait() {
	if (!m_impl) { return; }
	m_impl->drain_and_wait();
}

void Queue::drop_enqueued() {
	if (!m_impl) { return; }
	m_impl->drop_enqueued();
}
} // namespace task

auto task::get_max_threads() -> ThreadCount { return ThreadCount(std::thread::hardware_concurrency()); }
} // namespace klib

// log

#include "klib/log.hpp"
#include "klib/string/c_string.hpp"

namespace klib {
namespace log {
namespace {
[[maybe_unused]] constexpr auto to_filename(std::string_view path) -> std::string_view {
	auto const i = path.find_last_of("\\/");
	if (i == std::string_view::npos) { return path; }
	return path.substr(i + 1);
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

struct Storage {
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

	void print_to_file(CString const line) {
		auto lock = std::scoped_lock{m_mutex};
		m_file.print(line);
	}

	std::atomic<Level> max_level{Level::Debug};

  private:
	mutable std::mutex m_mutex{};
	FileImpl m_file{};
	std::optional<Colors> m_colors{colors_v};
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

auto log::get_thread_id() -> ThreadId {
	static auto s_id = std::atomic<std::underlying_type_t<ThreadId>>{};
	thread_local auto const ret = s_id++;
	return ThreadId{ret};
}

auto log::format(Input const& input) -> std::string {
	// [L] [tag/TT] message [timestamp] [source]

	static constexpr std::size_t reserve_v = debug_v ? 128 : 64;
	auto ret = std::string{};
	ret.reserve(input.message.size() + reserve_v);

	auto const level = level_to_char[input.level];
	auto const tid = std::underlying_type_t<ThreadId>(get_thread_id());
	auto const now = chr::time_point_cast<chr::seconds>(chr::system_clock::now());
	auto const timestamp = chr::zoned_time{chr::current_zone(), now};

	std::format_to(std::back_inserter(ret), "[{}] [{}/{:02}] {} [{:%T}]", level, input.tag, tid, input.message, timestamp);

	if constexpr (debug_v) { std::format_to(std::back_inserter(ret), " [{}:{}]", to_filename(input.file_name), input.line_number); }

	ret.append("\n");
	return ret;
}

void log::print(Input const& input) {
	if (input.level > g_storage.max_level) { return; }

	auto const text = format(input);

	auto* out = input.level == Level::Error ? stderr : stdout;
	auto const do_print = [&](std::optional<escape::Rgb> const rgb) {
		if (!rgb) {
			std::print("{}", text);
			return;
		}
		auto const fg = escape::foreground(*rgb);
		std::print(out, "{}{}{}", fg, text, escape::clear);
	};

	if (auto const colors = g_storage.get_colors()) {
		do_print((*colors)[input.level]);
	} else {
		do_print({});
	}
	std::fflush(out);

#if defined(_WIN32)
	OutputDebugStringA(text.c_str());
#endif

	g_storage.print_to_file(text.c_str());
}
} // namespace klib

// debug/trap

#include "klib/debug/trap.hpp"

namespace {
// https://gcc.gnu.org/pipermail/libstdc++/2025-May/061246.html
[[maybe_unused]] auto glibcxx_is_debugger_present() -> bool {
	using namespace std;
	string_view const prefix = "TracerPid:\t"; // populated since Linux 2.6.0
	ifstream in("/proc/self/status");
	string line;
	while (std::getline(in, line)) {
		if (!line.starts_with(prefix)) { continue; }

		string_view tracer = line;
		tracer.remove_prefix(prefix.size());
		if (tracer.size() == 1 && tracer[0] == '0') [[likely]] {
			return false; // Not being traced.
		}

		in.close();
		string_view cmd;
		string proc_dir = "/proc/" + string(tracer) + '/';
		in.open(proc_dir + "comm"); // since Linux 2.6.33
		if (std::getline(in, line)) [[likely]] {
			cmd = line;
		} else {
			in.close();
			in.open(proc_dir + "cmdline");
			if (std::getline(in, line)) {
				// NOLINTNEXTLINE(readability-redundant-string-cstr)
				cmd = line.c_str(); // Only up to first '\0'
			} else {
				return false;
			}
		}

		static constexpr auto known_debuggers = std::array{"gdb", "gdbserver", "lldb-server"};
		return std::ranges::any_of(known_debuggers, [cmd](char const* dbg) { return cmd.ends_with(dbg); });
	}
	return false;
}
} // namespace

auto klib::is_debugger_attached() -> bool {
#if defined(_WIN32)
	return IsDebuggerPresent() != 0;
#else
	return glibcxx_is_debugger_present();
#endif
}

// assert

#include "klib/debug/assert.hpp"

namespace klib {
namespace assertion {
namespace {
// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
auto g_fail_action = std::atomic<assertion::FailAction>{assertion::FailAction::Throw};

auto is_internal(std::string_view const trace_description) -> bool {
	static constexpr auto phrases_v = std::array{
		"!invoke_main+",
		"start_call_main",
		"register_frame_ctor",
	};
	if (trace_description.empty()) { return true; }
	return std::ranges::any_of(phrases_v, [trace_description](std::string_view const phrase) { return trace_description.contains(phrase); });
}
} // namespace
} // namespace assertion

auto assertion::get_fail_action() -> FailAction { return g_fail_action; }

void assertion::set_fail_action(FailAction const value) { g_fail_action = value; }

void assertion::append_trace(std::string& out, std::stacktrace const& trace) {
	if constexpr (use_stacktrace_v) {
		for (auto const& entry : trace) {
			auto const description = entry.description();
			if (is_internal(description)) { return; }
			std::format_to(std::back_inserter(out), "  {}", description);
			if constexpr (debug_v) {
				std::string const file = entry.source_file();
				if (!file.empty() && entry.source_line() > 0) { std::format_to(std::back_inserter(out), " [{}:{}]", file, entry.source_line()); }
			}
			out += '\n';
		}
	}
}

void assertion::print(std::string_view expr, std::stacktrace const& trace) {
	auto msg = std::format("assertion failed: '{}'\n", expr);
	append_trace(msg, trace);
	std::println(stderr, "{}", msg);
}

void assertion::trigger_failure() {
	switch (g_fail_action) {
	case FailAction::Throw: throw Failure{};
	case FailAction::Terminate: std::terminate(); return;
	default: return;
	}
}
} // namespace klib

// text_table

#include "klib/cli/text_table.hpp"

namespace klib {
void TextTable::push_row(std::vector<std::string> row) {
	m_rows.push_back(std::move(row));
	update_column_widths();
}

void TextTable::append_to(std::string& out) const {
	for (auto const& column : m_columns) { column.fmt = make_column_fmt(column); }

	static constexpr std::string_view per_column_spacing_v{"|  "};
	static constexpr std::string_view end_spacing_v{"|"};
	auto const spacing = (m_columns.size() * per_column_spacing_v.size()) + end_spacing_v.size();
	auto const total_width = std::accumulate(m_columns.begin(), m_columns.end(), spacing, [](std::size_t const s, Column const& c) { return s + c.max_width; });
	append_border(out, total_width);
	append_titles(out);
	append_separator(out, total_width);
	for (auto const& row : m_rows) {
		if (row.empty()) {
			append_separator(out, total_width);
			continue;
		}

		for (std::size_t i = 0; i < m_columns.size(); ++i) {
			auto const& column = m_columns.at(i);
			auto const cell = i < row.size() ? row.at(i) : std::string_view{};
			append_cell(out, column.fmt, cell);
		}

		if (!no_border) { out += '|'; }
		out += '\n';
	}
	append_border(out, total_width);
}

auto TextTable::serialize() const -> std::string {
	auto ret = std::string{};
	append_to(ret);
	return ret;
}

auto TextTable::make_column_fmt(Column const& column) const -> std::string {
	auto ret = std::string{};
	auto const align_char = [align = column.align] {
		switch (align) {
		default:
		case TextTable::Align::Left: return '<';
		case TextTable::Align::Right: return '>';
		case TextTable::Align::Center: return '^';
		}
	}();
	std::string_view const prefix = no_border ? "" : "| ";
	std::format_to(std::back_inserter(ret), "{}{{:{}{}}} ", prefix, align_char, column.max_width);
	return ret;
}

void TextTable::update_column_widths() {
	KLIB_ASSERT(!m_rows.empty());
	auto const& row = m_rows.back();
	if (row.empty()) { return; }
	for (auto [column, cell] : std::ranges::zip_view(m_columns, row)) { column.max_width = std::max(column.max_width, cell.size()); }
}

void TextTable::append_border(std::string& out, std::size_t const width) const {
	if (no_border || width < 2) { return; }
	out += '+';
	for (std::size_t i = 0; i < width - 2; ++i) { out += '-'; }
	out += "+\n";
}

void TextTable::append_separator(std::string& out, std::size_t const width) const {
	if (no_border) { return; }
	for (std::size_t i = 0; i < width; ++i) { out += '-'; }
	out += '\n';
}

void TextTable::append_cell(std::string& out, std::string_view const fmt, std::string_view const cell) {
	std::vformat_to(std::back_inserter(out), fmt, std::make_format_args(cell));
}

void TextTable::append_titles(std::string& out) const {
	for (auto const& column : m_columns) { append_cell(out, column.fmt, column.title); }
	if (!no_border) { out += '|'; }
	out += '\n';
}

auto TextTable::Builder::add_column(std::string title, Align const align) -> Builder& {
	auto column = Column{.title = std::move(title), .align = align};
	column.max_width = column.title.size();
	m_columns.push_back(std::move(column));
	return *this;
}

auto TextTable::Builder::build() const -> TextTable {
	auto ret = TextTable{};
	ret.m_columns = m_columns;
	return ret;
}
} // namespace klib

// vigenere

#include "klib/vigenere_cipher.hpp"
#include <functional>

namespace klib {
namespace {
class VigenereCipher {
  public:
	explicit constexpr VigenereCipher(std::string_view const key) : m_key(key) { KLIB_ASSERT(!m_key.empty()); }

	constexpr void encrypt(std::string_view const src, std::span<char> dst) const { process(src, dst, &shift); }
	constexpr void decrypt(std::string_view const src, std::span<char> dst) const { process(src, dst, &unshift); }

  private:
	static constexpr auto min_shift_v = 32;
	static constexpr auto max_shift_v = 126;
	static constexpr auto shift_mod_v = max_shift_v - min_shift_v + 1;

	[[nodiscard]] static constexpr auto in_range(char const ch) -> bool { return int(ch) >= min_shift_v && int(ch) <= max_shift_v; }

	[[nodiscard]] static constexpr auto shift(char const ch, int const distance) -> char {
		if (ch < min_shift_v || ch > max_shift_v) { return ch; }
		auto const ret = int(ch) - min_shift_v + distance;
		KLIB_ASSERT(ret >= 0);
		return char((ret % shift_mod_v) + min_shift_v);
	}

	[[nodiscard]] static constexpr auto unshift(char const ch, int const distance) -> char {
		if (ch < min_shift_v || ch > max_shift_v) { return ch; }
		auto const ret = int(ch) + shift_mod_v - min_shift_v - distance;
		KLIB_ASSERT(ret >= 0);
		return char((ret % shift_mod_v) + min_shift_v);
	}

	template <typename F>
	constexpr void process(std::string_view const src, std::span<char> dst, F func) const {
		KLIB_ASSERT(src.size() <= dst.size());
		if (src.empty()) { return; }
		for (std::size_t i = 0; i < src.size(); ++i) {
			auto const ch = src[i];
			if (in_range(ch)) {
				auto const j = i % m_key.size();
				auto const distance = int(m_key[j]) - min_shift_v;
				KLIB_ASSERT(distance >= 0);
				dst[i] = func(src[i], distance);
			} else {
				dst[i] = src[i];
			}
		}
	}

	std::string_view m_key;
};

template <typename F>
auto apply_vigenere(std::string_view const key, std::string_view const input, F func) -> std::string {
	auto ret = std::string{};
	ret.resize(input.size());
	auto const span = std::span{ret.data(), ret.size()};
	auto cipher = klib::VigenereCipher{key};
	std::invoke(func, &cipher, input, span);
	return ret;
}
} // namespace
} // namespace klib

auto klib::vigenere_encrypt(std::string_view const key, std::string_view const input) -> std::string {
	return apply_vigenere(key, input, &VigenereCipher::encrypt);
}

auto klib::vigenere_decrypt(std::string_view const key, std::string_view const input) -> std::string {
	return apply_vigenere(key, input, &VigenereCipher::decrypt);
}

// escape_code

#include "klib/string/escape_code.hpp"

namespace klib {
namespace escape {
namespace {
[[nodiscard]] auto colorify(Rgb const rgb, int const target) -> FixedString<> {
	return {"{}{};2;{};{};{}{}", prefix_v, target, rgb[0], rgb[1], rgb[2], suffix_v};
}
} // namespace
} // namespace escape

auto escape::foreground(Rgb const rgb) -> FixedString<> { return colorify(rgb, 38); }
auto escape::background(Rgb const rgb) -> FixedString<> { return colorify(rgb, 48); }
} // namespace klib

// from_chars

namespace klib {
auto FromChars::advance_if(char const ch) -> bool {
	if (text.empty() || text.front() != ch) { return false; }
	text.remove_prefix(1);
	return true;
}

auto FromChars::advance_if_any(std::string_view const chars) -> bool {
	if (chars.empty() || text.empty()) { return false; }
	return std::ranges::any_of(chars, [this](char const ch) { return advance_if(ch); });
}

auto FromChars::advance_if_all(std::string_view const str) -> bool {
	if (text.empty() || str.empty() || !text.starts_with(str)) { return false; }
	text.remove_prefix(str.size());
	return true;
}
} // namespace klib

// env

#include "klib/env.hpp"

#if defined(__linux__)
#include <linux/limits.h>
#include <unistd.h>
#endif

namespace klib {
auto env::exe_path() -> std::string const& {
	static std::string const ret = [] {
		auto ret = std::string{};
#if defined(_WIN32)
		auto buffer = std::array<char, MAX_PATH>{};
		DWORD length = GetModuleFileNameA(nullptr, buffer.data(), buffer.size());
		if (length == 0) { return ret; }
		ret = std::string{buffer.data(), length};
#elif defined(__linux__)
		auto buffer = std::array<char, PATH_MAX>{};
		ssize_t length = ::readlink("/proc/self/exe", buffer.data(), buffer.size());
		if (length == -1) { return ret; }
		ret = std::string{buffer.data(), std::size_t(length)};
#endif
		return ret;
	}();
	return ret;
}

auto env::get_var(CString const key) -> CString {
	if (key.as_view().empty()) { return {}; }
#if _WIN32 && __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
#endif
	// NOLINTNEXTLINE(concurrency-mt-unsafe)
	return std::getenv(key.c_str());
#if _WIN32 && __clang__
#pragma clang diagnostic pop
#endif
}

} // namespace klib

// demangle

#include "klib/demangle.hpp"

#if __has_include(<cxxabi.h>)
#define KLIB_USE_CXA_DEMANGLE
#include <cxxabi.h>
#endif

auto klib::demangled_name(std::type_info const& info) -> std::string {
#if defined(KLIB_USE_CXA_DEMANGLE)
	auto status = int{};
	auto const buf = std::unique_ptr<char, decltype(std::free)*>{
		abi::__cxa_demangle(info.name(), nullptr, nullptr, &status),
		std::free,
	};
	if (status == 0) { return buf.get(); }
#endif
	static constexpr auto prefixes_v = std::array{
		"struct ",
		"class ",
		"enum ",
	};
	auto view = std::string_view{info.name()};
	for (std::string_view const prefix : prefixes_v) {
		if (view.starts_with(prefix)) {
			view.remove_prefix(prefix.size());
			break;
		}
	}
	return std::string{view};
}

#undef KLIB_USE_CXA_DEMANGLE

// file_io

#include "klib/file_io.hpp"

namespace klib {
namespace fs = std::filesystem;
} // namespace klib

auto klib::resolve_symlink(std::string_view const path, int const max_iters) -> std::string {
	auto real_path = fs::path{path};
	auto err = std::error_code{};
	for (auto iter = 0; iter < max_iters; ++iter) {
		if (real_path.empty()) { return {}; }
		if (fs::is_symlink(real_path, err)) {
			real_path = fs::read_symlink(real_path, err);
			if (err != std::errc{}) { return {}; }
			continue;
		}

		return real_path.string();
	}
	return {};
}

// cli::prompt

#include "klib/cli/prompt.hpp"
#include "klib/debug/assert.hpp"
#include <iostream>
#include <print>
#include <string_view>

namespace klib {
using prompt::Option;
using prompt::Selection;

auto prompt::line(std::string_view const message, std::move_only_function<bool(std::string)> pred) -> Selection {
	static constexpr auto max_attempts_v{3};
	auto line = std::string{};
	for (auto attempt = 0; attempt < max_attempts_v; ++attempt) {
		std::print("\n{}\n> ", message);
		std::getline(std::cin, line);
		std::println();
		if (pred(std::move(line))) { return Selection::Line; }
		std::println(stderr, "invalid input");
	}
	return Selection::Invalid;
}

auto prompt::confirm(std::string_view const message) -> Selection {
	auto const msg = std::format("{} (y/N):", message);
	auto input = std::string{};
	auto const pred = [&](std::string in) {
		if (in.empty() || in == "n" || in == "y") {
			input = std::move(in);
			return true;
		}
		return false;
	};
	auto const selection = line(msg, pred);
	if (selection == Selection::Invalid) { return selection; }

	KLIB_ASSERT(selection == Selection::Line);
	if (input == "y") { return Selection::Confirm; }
	return Selection::Exit;
}

auto prompt::options(std::span<Option const> options, bool const empty_is_exit) -> Selection {
	auto message = std::string{};
	auto number = 0;
	for (auto const& option : options) { std::format_to(std::back_inserter(message), "{}) {}\n", ++number, option.text); }
	message += "q) quit";

	auto const pred = [&](std::string_view input) {
		if (input.empty()) {
			if (!empty_is_exit) { return false; }
			input = "q";
		}
		if (input == "q") {
			number = 0;
			return true;
		}
		if (!FromChars{.text = input}(number)) { return false; }

		return number >= 1 && number <= int(options.size());
	};

	auto const selection = line(message, pred);
	if (selection == Selection::Invalid) { return selection; }

	KLIB_ASSERT(selection == Selection::Line);
	if (number == 0) { return Selection::Exit; }
	KLIB_ASSERT(number > 0);
	auto const index = std::size_t(number - 1);
	auto const& option = options[index];
	if (option.callback) { option.callback(); }
	return Selection::Option;
}
} // namespace klib
