#pragma once
#include "klib/string/c_string.hpp"
#include <cstdlib>

namespace klib::shell {
enum struct ExitCode : int {};
inline constexpr auto success_v = ExitCode{EXIT_SUCCESS};
inline constexpr auto failure_v = ExitCode{EXIT_FAILURE};

constexpr std::string_view redirect_null_v =
#if defined(_WIN32)
	"NUL";
#else
	"/dev/null";
#endif

auto execute(CString expression) -> ExitCode;
auto execute(std::string_view expression, std::string_view redirect) -> ExitCode;
inline auto execute_silent(std::string_view const expression) -> ExitCode { return execute(expression, redirect_null_v); }
} // namespace klib::shell
