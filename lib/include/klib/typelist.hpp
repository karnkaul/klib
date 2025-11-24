#pragma once
#include "klib/meta.hpp"

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

static_assert(std::same_as<TypelistFront<Typelist<int, char, float>>, int>);
static_assert(std::same_as<TypelistBack<Typelist<int, char, float>>, float>);
} // namespace klib
