#pragma once
#include <concepts>

namespace klib {
template <typename Type>
	requires(std::floating_point<Type> || std::signed_integral<Type>)
constexpr auto abs(Type const t) -> Type {
	return t < Type(0) ? -t : t;
}
} // namespace klib
