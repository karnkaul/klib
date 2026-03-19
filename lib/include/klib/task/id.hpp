#pragma once
#include <cstdint>

namespace klib::task {
enum struct Id : std::uint64_t { None = 0 }; // NOLINT(performance-enum-size)
} // namespace klib::task
