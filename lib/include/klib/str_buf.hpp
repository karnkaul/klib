#pragma once
#include <array>
#include <cstring>
#include <span>
#include <string_view>

namespace klib {
template <std::size_t MaxLength>
using StrBuf = std::array<char, MaxLength + 1>;

constexpr auto copy_to(std::span<char> out, std::string_view const text) -> std::size_t {
	auto const size = text.size() > out.size() ? out.size() : text.size();
	for (std::size_t i = 0; i < size; ++i) { out[i] = text[i]; }
	return size;
}
} // namespace klib
