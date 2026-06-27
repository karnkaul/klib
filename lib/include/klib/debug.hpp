#pragma once

/// This header must be used in conjunction with module 'klib.core'.

#if defined(_MSC_VER) || defined(__MINGW64__)
#define KLIB_EXEC_DEBUG_TRAP() __debugbreak()
#elif __has_builtin(__builtin_debug_trap)
#define KLIB_EXEC_DEBUG_TRAP() __builtin_debug / trap()
#elif __has_include(<csignal>)
#include <csignal>
#define KLIB_EXEC_DEBUG_TRAP() std::raise(SIGTRAP)
#else
#define KLIB_EXEC_DEBUG_TRAP()
#endif

#define KLIB_DEBUG_TRAP()                                                                                                                                      \
	if (::klib::is_debugger_attached()) { KLIB_EXEC_DEBUG_TRAP(); }

#if defined(KLIB_USE_STACKTRACE)
#define KLIB_GET_TRACE std::stacktrace::current
#else
#define KLIB_GET_TRACE std::stacktrace
#endif

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
