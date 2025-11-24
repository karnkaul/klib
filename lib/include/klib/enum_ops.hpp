#pragma once
#include "klib/concepts.hpp"
#include <utility>

namespace klib {
template <EnumT E>
constexpr auto enable_enum_ops_v = false;

template <typename E>
concept EnumOpsT = EnumT<E> && enable_enum_ops_v<E>;
} // namespace klib

template <klib::EnumT E>
struct Or {
	constexpr auto operator()(E const a, E const b) const -> E { return E{std::underlying_type_t<E>(std::to_underlying(a) | std::to_underlying(b))}; }
};

template <klib::EnumT E>
struct And {
	constexpr auto operator()(E const a, E const b) const -> E { return E{std::underlying_type_t<E>(std::to_underlying(a) & std::to_underlying(b))}; }
};

template <klib::EnumT E>
struct Inv {
	constexpr auto operator()(E const e) const -> E { return E{std::underlying_type_t<E>(~std::to_underlying(e))}; }
};

template <klib::EnumOpsT E>
constexpr auto operator|=(E& out, E const b) -> E& {
	out = Or<E>{}(out, b);
	return out;
}

template <klib::EnumOpsT E>
constexpr auto operator|(E const a, E const b) -> E {
	auto ret = a;
	ret |= b;
	return ret;
}

template <klib::EnumOpsT E>
constexpr auto operator&=(E& out, E const b) -> E& {
	out = And<E>{}(out, b);
	return out;
}

template <klib::EnumOpsT E>
constexpr auto operator&(E const a, E const b) -> E {
	auto ret = a;
	ret &= b;
	return ret;
}

template <klib::EnumOpsT E>
constexpr auto operator~(E const e) -> E {
	return Inv<E>{}(e);
}
