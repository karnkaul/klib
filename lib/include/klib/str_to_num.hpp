#pragma once
#include "klib/concepts.hpp"
#include <charconv>
#include <string_view>

namespace klib {
template <NumberT Type>
[[nodiscard]] auto str_to_num(std::string_view const str, Type const& fallback = {}) -> Type {
	auto ret = Type{};
	auto [_, ec] = std::from_chars(str.data(), str.data() + str.size(), ret);
	if (ec != std::errc{}) { return fallback; }
	return ret;
}
} // namespace klib
