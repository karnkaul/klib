#pragma once
#include "klib/concepts.hpp"
#include <utility>

namespace klib {
template <EnumT E>
constexpr auto enable_enum_bitops(E&& /*unused*/) -> bool {
	return false;
}

template <typename E>
concept EnumOpsT = EnumT<E> && enable_enum_bitops(E{});
} // namespace klib

template <klib::EnumOpsT E>
constexpr auto operator|=(E& out, E const b) -> E& {
	out = E{std::underlying_type_t<E>(std::to_underlying(out) | std::to_underlying(b))};
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
	out = E{std::underlying_type_t<E>(std::to_underlying(out) & std::to_underlying(b))};
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
	return E{std::underlying_type_t<E>(~std::to_underlying(e))};
}
