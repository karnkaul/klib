#pragma once
#include <compare>
#include <cstdint>

namespace klib {
struct Version {
	std::int64_t major{};
	std::int64_t minor{};
	std::int64_t patch{};

	auto operator<=>(Version const&) const -> std::strong_ordering = default;
};
} // namespace klib
