#pragma once
#include <type_traits>

namespace klib {
template <typename... Ts>
class LargestOfT;

template <typename T>
class LargestOfT<T> {
  public:
	using type = T;
};

template <typename T, typename... Ts>
class LargestOfT<T, Ts...> {
	using Lhs = T;
	using Rhs = LargestOfT<Ts...>::type;

  public:
	using type = std::conditional_t<(sizeof(Lhs) >= sizeof(Rhs)), Lhs, Rhs>;
};

template <typename List>
using LargestOf = LargestOfT<List>::type;
} // namespace klib
