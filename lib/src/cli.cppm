export module klib.cli;

export import klib.core;

#pragma region prompt

export namespace klib::prompt {
enum class Selection : std::int8_t {
	Exit = 0,
	Invalid = 1,
	Option = 2,
	Confirm = Option,
	Line = Option,
};

struct Option {
	std::string_view text{};
	std::function<void()> callback{};
};

[[nodiscard]] auto line(std::string_view message, std::move_only_function<bool(std::string)> pred) -> prompt::Selection;
[[nodiscard]] auto confirm(std::string_view message) -> prompt::Selection;
[[nodiscard]] auto options(std::span<prompt::Option const> options, bool empty_is_exit) -> prompt::Selection;
} // namespace klib::prompt

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
		if (pred(std::move(line))) {
			return Selection::Line;
		}
		std::println(std::cerr, "invalid input");
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
	if (selection == Selection::Invalid) {
		return selection;
	}

	klib_assert(selection == Selection::Line);
	if (input == "y") {
		return Selection::Confirm;
	}
	return Selection::Exit;
}

auto prompt::options(std::span<Option const> options, bool const empty_is_exit) -> Selection {
	auto message = std::string{};
	auto number = 0;
	for (auto const& option : options) {
		std::format_to(std::back_inserter(message), "{}) {}\n", ++number, option.text);
	}
	message += "q) quit";

	auto const pred = [&](std::string_view input) {
		if (input.empty()) {
			if (!empty_is_exit) {
				return false;
			}
			input = "q";
		}
		if (input == "q") {
			number = 0;
			return true;
		}
		if (!FromChars{.text = input}(number)) {
			return false;
		}

		return number >= 1 && number <= int(options.size());
	};

	auto const selection = line(message, pred);
	if (selection == Selection::Invalid) {
		return selection;
	}

	klib_assert(selection == Selection::Line);
	if (number == 0) {
		return Selection::Exit;
	}
	klib_assert(number > 0);
	auto const index = static_cast<std::size_t>(number - 1);
	// NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)
	auto const& option = options[index];
	if (option.callback) {
		option.callback();
	}
	return Selection::Option;
}
} // namespace klib

#pragma endregion

#pragma region shell

export namespace klib::shell {
enum struct ExitCode : int {};
inline constexpr auto success_v = ExitCode{0};
inline constexpr auto failure_v = ExitCode{1};

inline constexpr std::string_view redirect_null_v =
#ifdef _WIN32
	"NUL";
#else
	"/dev/null";
#endif

auto execute(CString expression) -> ExitCode;
auto execute(std::string_view expression, std::string_view redirect) -> ExitCode;
inline auto execute_silent(std::string_view const expression) -> ExitCode {
	return execute(expression, redirect_null_v);
}

auto open_directory(CString path) -> ExitCode;
} // namespace klib::shell

namespace klib {
auto shell::execute(CString const expression) -> ExitCode {
	if (expression.as_view().empty()) {
		return success_v;
	}
	// NOLINTNEXTLINE(concurrency-mt-unsafe)
	return ExitCode(std::system(expression.c_str()));
}

auto shell::execute(std::string_view const expression, std::string_view const redirect) -> ExitCode {
	if (expression.empty()) {
		return success_v;
	}
	auto const redirected_expression = std::format("{} > {} 2>&1", expression, redirect);
	return execute(redirected_expression);
}

auto shell::open_directory(CString const path) -> ExitCode {
	static constexpr std::string_view command_v =
#ifdef _WIN32
		"explorer.exe";
#else
		"xdg-open";
#endif
	auto const expression = std::format(R"({} "{}")", command_v, path.as_view());
	return execute(expression);
}
} // namespace klib

#pragma endregion
