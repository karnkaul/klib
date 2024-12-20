#pragma once
#include <type_traits>

namespace klib {
template <template <typename, typename> typename Cmp, typename T, typename... Ts>
class TypeSelector;

template <template <typename, typename> typename Cmp, typename T>
class TypeSelector<Cmp, T> {
  public:
	using type = T;
};

template <template <typename, typename> typename Cmp, typename T, typename... Ts>
class TypeSelector {
	using Lhs = T;
	using Rhs = TypeSelector<Cmp, Ts...>::type;

  public:
	using type = Cmp<Lhs, Rhs>::type;
};

template <typename T, typename U>
struct SmallerType {
	using type = std::conditional_t<(sizeof(T) <= sizeof(U)), T, U>;
};

template <typename T, typename U>
struct LargerType {
	using type = std::conditional_t<(sizeof(T) >= sizeof(U)), T, U>;
};

template <typename T, typename... Ts>
using LargestOf = TypeSelector<LargerType, T, Ts...>::type;

template <typename T, typename... Ts>
using SmallestOf = TypeSelector<SmallerType, T, Ts...>::type;
} // namespace klib
