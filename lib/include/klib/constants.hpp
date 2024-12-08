#pragma once
#include <cstdint>

namespace klib {
constexpr std::uint64_t kibi_v = 1024;
constexpr std::uint64_t mebi_v = 1024 * kibi_v;
constexpr std::uint64_t gibi_v = 1024 * mebi_v;
constexpr std::uint64_t tebi_v = 1024 * gibi_v;

constexpr std::uint64_t kilo_v = 1000;
constexpr std::uint64_t mega_v = 1000 * kilo_v;
constexpr std::uint64_t giga_v = 1000 * mega_v;
constexpr std::uint64_t tera_v = 1000 * giga_v;

constexpr auto debug_v =
#if defined(KLIB_DEBUG)
	true;
#else
	false;
#endif

constexpr auto use_stacktrace_v =
#if defined(KLIB_USE_STACKTRACE)
	true;
#else
	false;
#endif
} // namespace klib
