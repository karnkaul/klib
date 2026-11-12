#include "klib/cli/prompt.hpp"
#include "klib/cli/shell.hpp"
#include "klib/core/assert.hpp"
#include "klib/core/string/from_chars.hpp"
#include <iostream>
#include <print>
#include <string_view>

#pragma region cli_prompt

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
