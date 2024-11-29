#pragma once
#include <cstddef>
#include <cstdint>

namespace klib::task {
enum struct ThreadCount : std::uint8_t { Minimum = 1 };
enum struct ElementCount : std::size_t { Unbounded = 0 };

[[nodiscard]] auto get_max_threads() -> ThreadCount;

struct QueueCreateInfo {
	ThreadCount thread_count{get_max_threads()};
	ElementCount max_elements{ElementCount::Unbounded};
};
} // namespace klib::task
