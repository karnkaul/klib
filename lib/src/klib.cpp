#include <algorithm>
#include <array>
#include <atomic>
#include <cassert>
#include <condition_variable>
#include <deque>
#include <iomanip>
#include <mutex>
#include <print>
#include <sstream>
#include <thread>
#include <utility>
#include <vector>

// unit_test

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
	auto discard = char{'.'};
	auto str = std::istringstream{text.c_str()};
	str >> ret.major >> discard >> ret.minor >> discard >> ret.patch;
	return ret;
}

// task

#include <klib/task/queue.hpp>

namespace klib::task {
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

auto Queue::get_max_threads() -> ThreadCount { return ThreadCount(std::thread::hardware_concurrency()); }

Queue::Queue(CreateInfo create_info) {
	if (create_info.thread_count == ThreadCount::Default) { create_info.thread_count = ThreadCount(get_max_threads()); }
	create_info.thread_count = std::clamp(create_info.thread_count, ThreadCount::Minimum, get_max_threads());
	m_impl.reset(new Impl(create_info)); // NOLINT(cppcoreguidelines-owning-memory)
}

auto Queue::thread_count() const -> ThreadCount {
	if (!m_impl) { return ThreadCount::Default; }
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
} // namespace klib::task

// arg*

#include <args/parser.hpp>
#include <klib/args/parse.hpp>

namespace klib {
namespace args {
Arg::Arg(bool& out, std::string_view const key, std::string_view const help_text)
	: m_param(ParamOption{Binding::create<bool>(), &out, true, to_letter(key), to_word(key), help_text}) {}

Arg::Arg(std::span<Arg const> args, std::string_view const name, std::string_view const help_text) : m_param(ParamCommand{args, name, help_text}) {}

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

	m_args = cmd->args;
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

auto args::parse_args(ParseInfo const& info, std::span<Arg const> args, int argc, char const* const* argv) -> ParseResult {
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
