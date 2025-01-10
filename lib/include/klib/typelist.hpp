#pragma once

#include <concepts>

namespace klib {
template <typename... Ts>
struct Typelist {
	static constexpr auto count_v = sizeof...(Ts);
};

template <typename List>
struct TypelistFrontT;

template <typename T, typename... Ts>
struct TypelistFrontT<Typelist<T, Ts...>> {
	using type = T;
};

template <typename List>
using TypelistFront = TypelistFrontT<List>::type;

template <typename List>
struct TypelistBackT;

template <typename T>
struct TypelistBackT<Typelist<T>> {
	using type = T;
};

template <typename T, typename... Ts>
struct TypelistBackT<Typelist<T, Ts...>> {
	using type = TypelistBackT<Typelist<Ts...>>::type;
};

template <typename List>
using TypelistBack = TypelistBackT<List>::type;

template <typename List>
class LargestOfT;

template <typename T>
class LargestOfT<Typelist<T>> {
  public:
	using type = T;
};

template <typename T, typename... Ts>
class LargestOfT<Typelist<T, Ts...>> {
	using Lhs = T;
	using Rhs = LargestOfT<Typelist<Ts...>>::type;

  public:
	using type = std::conditional_t<(sizeof(Lhs) >= sizeof(Rhs)), Lhs, Rhs>;
};

template <typename List>
using LargestOf = LargestOfT<List>::type;

static_assert(std::same_as<TypelistFront<Typelist<int, char, float>>, int>);
static_assert(std::same_as<TypelistBack<Typelist<int, char, float>>, float>);
static_assert(std::same_as<LargestOf<Typelist<int, double>>, double>);
} // namespace klib
