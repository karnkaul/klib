#pragma once
#include <klib/fixed_string.hpp>
#include <array>
#include <cstdint>

namespace klib::escape {
inline constexpr auto prefix_v = std::string_view{"\x1b["};
inline constexpr auto suffix_v = std::string_view{"m"};

using Rgb = std::array<std::uint8_t, 3>;

inline auto const clear = FixedString<>{"{}{}", prefix_v, suffix_v};

[[nodiscard]] auto foreground(Rgb rgb) -> FixedString<>;
[[nodiscard]] auto background(Rgb rgb) -> FixedString<>;
} // namespace klib::escape
