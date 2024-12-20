#include <algorithm>
#include <array>
#include <atomic>
#include <cassert>
#include <chrono>
#include <condition_variable>
#include <deque>
#include <fstream>
#include <iomanip>
#include <mutex>
#include <numeric>
#include <print>
#include <ranges>
#include <sstream>
#include <thread>
#include <utility>
#include <vector>

#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#define NOCOMM
#define NOMINMAX
#define STRICT
#define UNICODE
#include <Windows.h>
#endif

namespace chr = std::chrono;

// common
namespace {
[[maybe_unused]] constexpr auto is_digit(char const c) { return c >= '0' && c <= '9'; };

[[maybe_unused]] auto get_line_containing(char const* path, std::string_view const substr) -> std::string {
	auto file = std::ifstream{path};
	if (!file.is_open()) { return {}; }

	for (auto line = std::string{}; std::getline(file, line);) {
		auto const i = line.find(substr);
		if (i == std::string::npos) { continue; }
		auto view = std::string_view{line}.substr(i + substr.size());
		while (!view.empty() && std::isspace(static_cast<unsigned char>(view.front())) != 0) { view = view.substr(1); }
		return std::string{view};
	}

	return {};
}

template <typename Pred>
constexpr auto trim_until(std::string_view text, Pred pred) -> std::string_view {
	while (!text.empty() && !pred(text.front())) { text = text.substr(1); }
	return text;
}

template <std::integral T>
auto to_int(std::string_view const text, T const fallback = {}) -> T {
	auto ret = T{};
	auto const [_, ec] = std::from_chars(text.data(), text.data() + text.size(), ret);
	if (ec != std::errc{}) { return fallback; }
	return ret;
}
} // namespace

// unit test

#include <klib/unit_test.hpp>

namespace klib {
namespace {
struct Assert {};

struct State {
	std::vector<test::TestCase*> tests{};
	bool failure{};

	static auto self() -> State& {
		static auto ret = State{};
		return ret;
	}
};

void check_failed(std::string_view const type, std::string_view const expr, std::string_view const file, int const line) {
	std::println(stderr, "{} failed: '{}' [{}:{}]", type, expr, file, line);
	State::self().failure = true;
}
} // namespace

void test::check_expect(bool const pred, std::string_view const expr, std::string_view const file, int const line) {
	if (pred) { return; }
	check_failed("expectation", expr, file, line);
}

void test::check_assert(bool const pred, std::string_view const expr, std::string_view const file, int const line) {
	if (pred) { return; }
	check_failed("assertion", expr, file, line);
	throw Assert{};
}

auto test::run_tests() -> int {
	auto& state = State::self();
	for (auto* test : state.tests) {
		try {
			std::println("[{}]", test->name);
			test->run();
		} catch (Assert const& /*a*/) {}
	}

	if (state.failure) {
		std::println("FAILED");
		return EXIT_FAILURE;
	}

	std::println("passed");
	return EXIT_SUCCESS;
}

namespace test {
TestCase::TestCase(std::string_view const name) : name(name) { State::self().tests.push_back(this); }
} // namespace test
} // namespace klib

// version

#include <klib/version_str.hpp>

void klib::append_to(std::string& out, Version const& version) {
	std::format_to(std::back_inserter(out), "v{}.{}.{}", version.major, version.minor, version.patch);
}

auto klib::to_string(Version const& version) -> std::string {
	auto ret = std::string{};
	append_to(ret, version);
	return ret;
}

auto klib::to_version(CString text) -> Version {
	if (text.as_view().starts_with('v')) { text = CString{text.as_view().substr(1).data()}; }
	if (text.as_view().empty()) { return {}; }

	auto ret = Version{};
	auto discard = char{};
	auto str = std::istringstream{text.c_str()};
	str >> ret.major >> discard >> ret.minor >> discard >> ret.patch;
	return ret;
}

// task

#include <klib/task/queue.hpp>

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

// args

#include <args/parser.hpp>
#include <klib/args/parse.hpp>

namespace klib {
namespace args {
Arg::Arg(bool& out, std::string_view const key, std::string_view const help_text)
	: m_param(ParamOption{Binding::create<bool>(), &out, true, to_letter(key), to_word(key), help_text}) {}

Arg::Arg(std::span<Arg const> args, std::string_view const name, std::string_view const help_text)
	: m_param(ParamCommand{args.data(), args.size(), name, help_text}) {}

namespace {
constexpr auto get_exe_name(std::string_view const arg0) -> std::string_view {
	auto const i = arg0.find_last_of("\\/");
	if (i == std::string_view::npos) { return arg0; }
	return arg0.substr(i + 1);
}

struct ErrorPrinter {
	ErrorPrinter(ErrorPrinter const&) = delete;
	ErrorPrinter(ErrorPrinter&&) = delete;
	auto operator=(ErrorPrinter const&) = delete;
	auto operator=(ErrorPrinter&&) = delete;

	explicit ErrorPrinter(std::string_view exe_name, std::string_view cmd_id = {}) : exe_name(exe_name), cmd_name(cmd_id) {
		str.reserve(400);
		append_error_prefix();
	}

	~ErrorPrinter() {
		if (helpline) { append_helpline(); }
		std::print(stderr, "{}", str);
	}

	[[nodiscard]] auto invalid_value(std::string_view const input, std::string_view const value) -> ParseError {
		helpline = false;
		std::format_to(std::back_inserter(str), "invalid {}: '{}'\n", input, value);
		return ParseError::InvalidArgument;
	}

	[[nodiscard]] auto invalid_option(char const letter) -> ParseError {
		std::format_to(std::back_inserter(str), "invalid option -- '{}'\n", letter);
		return ParseError::InvalidOption;
	}

	[[nodiscard]] auto unrecognized_option(std::string_view const input) -> ParseError {
		std::format_to(std::back_inserter(str), "unrecognized option '--{}'\n", input);
		return ParseError::InvalidOption;
	}

	[[nodiscard]] auto unrecognized_command(std::string_view const input) -> ParseError {
		std::format_to(std::back_inserter(str), "unrecognized command '{}'\n", input);
		return ParseError::InvalidCommand;
	}

	[[nodiscard]] auto extraneous_argument(std::string_view const input) -> ParseError {
		std::format_to(std::back_inserter(str), "extraneous argument '{}'\n", input);
		return ParseError::InvalidArgument;
	}

	[[nodiscard]] auto option_requires_argument(std::string_view const input) -> ParseError {
		if (input.size() == 1) {
			std::format_to(std::back_inserter(str), "option requires an argument -- '{}'\n", input);
		} else {
			std::format_to(std::back_inserter(str), "option '{}' requires an argument\n", input);
		}
		return ParseError::MissingArgument;
	}

	[[nodiscard]] auto option_is_flag(std::string_view const input) -> ParseError {
		if (input.size() == 1) {
			std::format_to(std::back_inserter(str), "option does not take an argument -- '{}'\n", input);
		} else {
			std::format_to(std::back_inserter(str), "option '{}' does not take an argument\n", input);
		}
		return ParseError::InvalidArgument;
	}

	[[nodiscard]] auto missing_argument(std::string_view name) -> ParseError {
		std::format_to(std::back_inserter(str), "missing {}\n", name);
		return ParseError::MissingArgument;
	}

	void append_error_prefix() {
		str += exe_name;
		if (!cmd_name.empty()) { std::format_to(std::back_inserter(str), " {}", cmd_name); }
		str += ": ";
	}

	void append_helpline() {
		std::format_to(std::back_inserter(str), "Try '{}", exe_name);
		if (!cmd_name.empty()) { std::format_to(std::back_inserter(str), " {}", cmd_name); }
		str += " --help' for more information.\n";
	}

	std::string_view exe_name{};
	std::string_view cmd_name{};
	bool helpline{true};
	std::string str;
};

struct PrintParam {
	std::ostream& out;
	bool* has_commands{};

	void operator()(ParamOption const& o) const {
		out << " [";
		if (o.letter != '\0') {
			out << '-' << o.letter;
			if (!o.word.empty()) { out << '|'; }
		}
		if (!o.word.empty()) { out << "--" << o.word; }
		if (!o.is_flag) { out << "(=" << o.to_string() << ')'; }
		out << ']';
	}

	void operator()(ParamPositional const& p) const {
		std::string_view const wrap = p.is_required() ? "<>" : "[]";
		out << ' ' << wrap[0] << p.name;
		if (!p.is_list && !p.is_required()) { out << "(=" << p.to_string() << ")"; }
		out << wrap[1];
	}

	void operator()(ParamCommand const& /*c*/) const {
		if (has_commands != nullptr) { *has_commands = true; }
	}
};

auto append_exe_cmd(std::ostream& out, std::string_view const exe, std::string_view const cmd) {
	out << exe;
	if (!cmd.empty()) { out << " " << cmd; }
}

void append_positionals(std::ostream& out, std::span<Arg const> args) {
	for (auto const& arg : args) {
		if (auto const* pos = std::get_if<ParamPositional>(&arg.get_param())) { PrintParam{out}(*pos); }
	}
}

void append_option_list(std::ostream& out, std::size_t const width, std::span<Arg const> args) {
	out << "\nOPTIONS\n";
	auto const print_option = [&out, width](std::string_view const key, std::string_view const help_text) {
		out << "  " << std::setw(int(width)) << key << help_text << "\n";
	};
	auto option_key = std::string{};
	for (auto const& arg : args) {
		auto const* option = std::get_if<ParamOption>(&arg.get_param());
		if (option == nullptr) { continue; }
		option_key.clear();
		if (option->letter == '\0') {
			option_key += "    ";
		} else {
			option_key += '-';
			option_key += option->letter;
			if (!option->word.empty()) { option_key += ", "; }
		}
		if (!option->word.empty()) {
			option_key += "--";
			option_key += option->word;
		}
		print_option(option_key, option->help_text);
	}
	print_option("    --help", "display this help and exit");
	print_option("    --usage", "print usage and exit");
	print_option("    --version", "print version text and exit");
}

void append_command_list(std::ostream& out, std::size_t const width, std::span<Arg const> args) {
	out << "\nCOMMANDS\n" << std::left;
	for (auto const& arg : args) {
		auto const* cmd = std::get_if<ParamCommand>(&arg.get_param());
		if (cmd == nullptr) { continue; }
		out << "  " << std::setw(int(width)) << cmd->name << cmd->help_text << "\n";
	}
}

void print_help(ParseInfo const& info, std::string_view const exe, std::string_view const cmd, std::span<Arg const> args) {
	auto out = std::ostringstream{};
	if (!info.help_text.empty()) { out << info.help_text << "\n"; }

	auto const has_version = !info.version.empty();
	auto has_positionals = false;
	auto has_options = false;
	auto options_width = std::string_view{has_version ? "___--version" : "___--usage"}.size();
	auto commands_width = std::size_t{};
	for (auto const& arg : args) {
		switch (arg.get_param().index()) {
		case 0:
			has_options = true;
			options_width = std::max(options_width, std::get<ParamOption>(arg.get_param()).word.size() + 6);
			break;
		case 1: has_positionals = true; break;
		case 2: commands_width = std::max(commands_width, std::get<ParamCommand>(arg.get_param()).name.size()); break;
		default: std::unreachable(); break;
		}
	}
	auto const has_commands = commands_width > 0;

	out << "Usage:\n  ";
	append_exe_cmd(out, exe, cmd);

	if (has_options) { out << " [OPTION...]"; }
	if (has_commands) {
		out << " <COMMAND> [COMMAND_ARGS...]";
	} else if (has_positionals) {
		append_positionals(out, args);
	}
	out << "\n  ";
	append_exe_cmd(out, exe, cmd);
	if (has_commands) { out << " [COMMAND]"; }
	out << " [--help|--usage";
	if (has_version) { out << "|--version"; }
	out << "]\n" << std::left;

	append_option_list(out, options_width + 4, args);

	if (has_commands) { append_command_list(out, commands_width + 4, args); }

	if (!info.epilogue.empty()) { out << "\n" << info.epilogue << "\n"; }
	out << std::right;

	std::println("{}", out.str());
}

void print_usage(std::string_view const exe, std::string_view const cmd, std::span<Arg const> args) {
	auto out = std::ostringstream{};
	append_exe_cmd(out, exe, cmd);
	auto has_commands = false;
	auto const print_param = PrintParam{out, &has_commands};
	for (auto const& arg : args) { std::visit(print_param, arg.get_param()); }

	if (has_commands) { out << " <COMMAND> [COMMAND_ARGS...]"; }

	std::println("{}", out.str());
}
} // namespace

auto Parser::parse(std::span<Arg const> args) -> ParseResult {
	m_args = args;
	m_cursor = {};
	m_has_commands = std::ranges::any_of(m_args, [](Arg const& a) { return std::holds_alternative<ParamCommand>(a.get_param()); });

	auto result = ParseResult{};

	while (m_scanner.next()) {
		result = parse_next();
		if (result.early_return()) { return result; }
	}

	result = check_required();
	if (result.early_return()) { return result; }

	if (m_cursor.cmd != nullptr) { return m_cursor.cmd->name; }

	return result;
}

auto Parser::select_command() -> ParseResult {
	auto const name = m_scanner.get_value();
	auto const* cmd = find_command(name);
	if (cmd == nullptr) { return ErrorPrinter{m_exe_name}.unrecognized_command(name); }

	m_args = {cmd->arg_ptr, cmd->arg_count};
	m_cursor = Cursor{.cmd = cmd};
	return {};
}

auto Parser::parse_next() -> ParseResult {
	switch (m_scanner.get_token_type()) {
	case TokenType::Argument: return parse_argument();
	case TokenType::Option: return parse_option();
	case TokenType::ForceArgs: return {};
	default:
	case TokenType::None: std::unreachable(); return {};
	}

	return {};
}

auto Parser::parse_option() -> ParseResult {
	switch (m_scanner.get_option_type()) {
	case OptionType::Letters: return parse_letters();
	case OptionType::Word: return parse_word();
	default:
	case OptionType::None: break;
	}

	std::unreachable();
	return {};
}

auto Parser::parse_letters() -> ParseResult {
	auto letter = char{};
	auto is_last = false;
	while (m_scanner.next_letter(letter, is_last)) {
		auto const* option = find_option(letter);
		if (option == nullptr) { return ErrorPrinter{m_exe_name, get_cmd_name()}.invalid_option(letter); }
		if (!is_last) {
			if (!option->is_flag) { return ErrorPrinter{m_exe_name, get_cmd_name()}.option_requires_argument({&letter, 1}); }
			[[maybe_unused]] auto const unused = option->assign({});
		} else {
			return parse_last_option(*option, {&letter, 1});
		}
	}

	return {};
}

auto Parser::parse_word() -> ParseResult {
	auto const word = m_scanner.get_key();
	if (try_builtin(word)) { return ExecutedBuiltin{}; }
	auto const* option = find_option(word);
	if (option == nullptr) { return ErrorPrinter{m_exe_name, get_cmd_name()}.unrecognized_option(word); }
	return parse_last_option(*option, word);
}

auto Parser::parse_last_option(ParamOption const& option, std::string_view input) -> ParseResult {
	if (option.is_flag) {
		if (!m_scanner.get_value().empty()) { return ErrorPrinter{m_exe_name, get_cmd_name()}.option_is_flag(input); }
		[[maybe_unused]] auto const unused = option.assign({});
		return {};
	}

	auto value = m_scanner.get_value();
	if (value.empty()) {
		if (m_scanner.peek() != TokenType::Argument) { return ErrorPrinter{m_exe_name, get_cmd_name()}.option_requires_argument(input); }
		m_scanner.next();
		value = m_scanner.get_value();
	}
	if (!option.assign(value)) { return ErrorPrinter{m_exe_name, get_cmd_name()}.invalid_value(input, value); }

	return {};
}

auto Parser::parse_argument() -> ParseResult {
	if (m_has_commands && m_cursor.cmd == nullptr) { return select_command(); }
	return parse_positional();
}

auto Parser::parse_positional() -> ParseResult {
	auto const* pos = next_positional();
	if (pos == nullptr) { return ErrorPrinter{m_exe_name, get_cmd_name()}.extraneous_argument(m_scanner.get_value()); }
	if (!pos->assign(m_scanner.get_value())) { return ErrorPrinter{m_exe_name, get_cmd_name()}.invalid_value(pos->name, m_scanner.get_value()); }
	return {};
}

auto Parser::try_builtin(std::string_view const word) const -> bool {
	if (!m_info.version.empty() && word == "version") {
		std::println("{}", m_info.version);
		return true;
	}

	if (word == "help") {
		auto info = m_info;
		info.help_text = get_help_text();
		print_help(info, m_exe_name, get_cmd_name(), m_args);
		return true;
	}

	if (word == "usage") {
		print_usage(m_exe_name, get_cmd_name(), m_args);
		return true;
	}

	return false;
}

auto Parser::find_option(char const letter) const -> ParamOption const* {
	for (auto const& arg : m_args) {
		auto const* ret = std::get_if<ParamOption>(&arg.get_param());
		if (ret != nullptr && ret->letter == letter) { return ret; }
	}
	return nullptr;
}

auto Parser::find_option(std::string_view const word) const -> ParamOption const* {
	for (auto const& arg : m_args) {
		auto const* ret = std::get_if<ParamOption>(&arg.get_param());
		if (ret != nullptr && ret->word == word) { return ret; }
	}
	return nullptr;
}

auto Parser::find_command(std::string_view const name) const -> ParamCommand const* {
	for (auto const& arg : m_args) {
		auto const* ret = std::get_if<ParamCommand>(&arg.get_param());
		if (ret != nullptr && ret->name == name) { return ret; }
	}
	return nullptr;
}

auto Parser::next_positional() -> ParamPositional const* {
	auto& index = m_cursor.next_pos;
	for (; index < m_args.size(); ++index) {
		auto const& arg = m_args[index];
		auto const* ret = std::get_if<ParamPositional>(&arg.get_param());
		if (ret != nullptr) {
			if (!ret->is_list) { ++index; }
			return ret;
		}
	}
	return nullptr;
}

auto Parser::check_required() -> ParseResult {
	if (m_has_commands && m_cursor.cmd == nullptr) { return ErrorPrinter{m_exe_name}.missing_argument("command"); }

	for (auto const* p = next_positional(); p != nullptr; p = next_positional()) {
		if (p->is_required()) { return ErrorPrinter{m_exe_name, get_cmd_name()}.missing_argument(p->name); }
		if (p->is_list) { return {}; }
	}

	return {};
}
} // namespace args

auto args::parse(ParseInfo const& info, std::span<Arg const> args, int argc, char const* const* argv) -> ParseResult {
	auto exe_name = std::string_view{"<app>"};
	auto cli_args = std::span{argv, std::size_t(argc)};
	if (!cli_args.empty()) {
		exe_name = get_exe_name(cli_args.front());
		cli_args = cli_args.subspan(1);
	};
	auto parser = Parser{info, exe_name, cli_args};
	return parser.parse(args);
}
} // namespace klib

// log

#include <klib/log.hpp>

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
	using Lock = std::unique_lock<std::mutex>;

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

	void print_to_file(CString const line) {
		auto lock = std::scoped_lock{m_mutex};
		m_file.print(line);
	}

	std::atomic<Level> max_level{Level::Debug};

  private:
	mutable std::mutex m_mutex{};
	FileImpl m_file{};
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
	std::print(out, "{}", text);

#if defined(_WIN32)
	OutputDebugStringA(text.c_str());
#endif

	g_storage.print_to_file(text.c_str());
}
} // namespace klib

// debug_trap

#include <klib/debug_trap.hpp>

auto klib::is_debugger_attached() -> bool {
#if defined(_WIN32)
	return IsDebuggerPresent() != 0;
#else
	auto const tpid_line = get_line_containing("/proc/self/status", "TracerPid:");
	auto const tracer_pid = trim_until(tpid_line, &is_digit);
	return to_int<std::int64_t>(tracer_pid, -1) > 0;
#endif
}

// assert

#include <klib/assert.hpp>

namespace klib {
namespace assertion {
namespace {
// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
auto g_fail_action = std::atomic<assertion::FailAction>{assertion::FailAction::Throw};

auto is_internal(std::string_view const trace_description) -> bool {
	static constexpr auto phrases_v = std::array{
		"!invoke_main+",
		"start_call_main",
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
			std::format_to(std::back_inserter(out), "  {} [{}:{}]\n", description, entry.source_file(), entry.source_line());
		}
	}
}

void assertion::print(std::string_view expr, std::stacktrace const& trace) noexcept(false) {
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

#include <klib/text_table.hpp>

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
