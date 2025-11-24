#pragma once
#include "klib/concepts.hpp"
#include <array>
#include <bit>
#include <cstddef>

namespace klib {
template <MemcpyAble Type>
constexpr auto byte_cast(Type const& t) -> std::array<std::byte, sizeof(Type)> {
	return std::bit_cast<std::array<std::byte, sizeof(Type)>>(t);
}
} // namespace klib
