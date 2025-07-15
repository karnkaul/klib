#pragma once
#include <klib/debug_trap.hpp>
#include <cstdint>
#include <exception>
#include <stacktrace>

namespace klib::assertion {
enum class FailAction : std::uint8_t { Throw, Terminate, None };

struct Failure : std::exception {
	[[nodiscard]] auto what() const noexcept -> char const* final { return "assertion failure"; }
};

[[nodiscard]] auto get_fail_action() -> FailAction;
void set_fail_action(FailAction value);

#if defined(KLIB_USE_STACKTRACE)
#define KLIB_GET_TRACE std::stacktrace::current
#else
#define KLIB_GET_TRACE std::stacktrace
#endif

void append_trace(std::string& out, std::stacktrace const& trace);
void print(std::string_view expr, std::stacktrace const& trace = KLIB_GET_TRACE());

void trigger_failure();
} // namespace klib::assertion

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define KLIB_ASSERT(expr)                                                                                                                                      \
	if (!bool(expr)) {                                                                                                                                         \
		::klib::assertion::print(#expr);                                                                                                                       \
		KLIB_DEBUG_TRAP();                                                                                                                                     \
		::klib::assertion::trigger_failure();                                                                                                                  \
	}
#if defined(KLIB_DEBUG)
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define KLIB_ASSERT_DEBUG(expr) KLIB_ASSERT(expr)
#else
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define KLIB_ASSERT_DEBUG(expr)
#endif
