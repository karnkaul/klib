#pragma once
#include <klib/debug_trap.hpp>
#include <exception>
#include <stacktrace>

namespace klib {
struct AssertionFailure : std::exception {
	[[nodiscard]] auto what() const noexcept -> char const* final { return "assertion failure"; }
};

void append_trace(std::string& out, std::stacktrace const& trace);

void print_assertion_failure(std::string_view expr, std::stacktrace const& trace = std::stacktrace::current()) noexcept(false);
} // namespace klib

#if defined(KLIB_DEBUG)
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define KLIB_ASSERT(expr)                                                                                                                                      \
	if (!(expr)) {                                                                                                                                             \
		::klib::print_assertion_failure(#expr);                                                                                                                \
		KLIB_DEBUG_TRAP();                                                                                                                                     \
		throw ::klib::AssertionFailure{};                                                                                                                      \
	}
#else
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define KLIB_ASSERT(expr)
#endif
