#pragma once
#include <array>
#include <cstring>
#include <ranges>
#include <span>
#include <string_view>

namespace klib {
template <std::size_t MaxLength>
using StrBuf = std::array<char, MaxLength + 1>;

constexpr auto copy_to(std::span<char> out, std::string_view const text) -> std::size_t {
	auto ret = 0uz;
	for (auto [o, i] : std::views::zip(out, text)) {
		o = i;
		++ret;
	}
	return ret;
}
} // namespace klib
