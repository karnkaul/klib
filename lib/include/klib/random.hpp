#pragma once
#include <concepts>
#include <random>

namespace klib {
template <std::integral Type = int, typename Gen>
[[nodiscard]] auto random_int(Gen& generator, Type const lo, Type const hi) -> Type {
	return std::uniform_int_distribution<Type>{lo, hi}(generator);
}

template <std::integral Type = std::size_t, typename Gen>
[[nodiscard]] auto random_index(Gen& generator, Type const size) -> Type {
	if (size == Type(0)) { return Type{}; }
	return random_int<Type>(generator, 0, size - 1);
}

template <std::floating_point Type = float, typename Gen>
[[nodiscard]] auto random_float(Gen& generator, Type const lo, Type const hi) -> Type {
	return std::uniform_real_distribution<Type>{lo, hi}(generator);
}
} // namespace klib
