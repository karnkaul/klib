#pragma once

#if defined(_MSC_VER) || defined(__MINGW64__)
#define KLIB_EXEC_DEBUG_TRAP() __debugbreak()
#elif __has_builtin(__builtin_debug_trap)
#define KLIB_EXEC_DEBUG_TRAP() __builtin_debug_trap()
#elif __has_include(<csignal>)
#include <csignal>
#define KLIB_EXEC_DEBUG_TRAP() std::raise(SIGTRAP)
#else
#define KLIB_EXEC_DEBUG_TRAP()
#endif

#define KLIB_DEBUG_TRAP()                                                                                                                                      \
	if (::klib::is_debugger_attached()) { KLIB_EXEC_DEBUG_TRAP(); }

namespace klib {
[[nodiscard]] auto is_debugger_attached() -> bool;
} // namespace klib
