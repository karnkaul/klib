#pragma once
#include "klib/concepts.hpp"

namespace klib {
template <EnumT E>
class EnumFlags {
  public:
	using value_type = std::underlying_type_t<E>;

	EnumFlags() = default;

	template <std::same_as<E>... Es>
	explicit(false) constexpr EnumFlags(Es const... es) : m_bits((... | static_cast<value_type>(es))) {}

	constexpr auto operator|=(EnumFlags const rhs) -> EnumFlags& {
		m_bits |= rhs.m_bits;
		return *this;
	}

	constexpr auto operator&=(EnumFlags const rhs) -> EnumFlags& {
		m_bits &= rhs.m_bits;
		return *this;
	}

	friend constexpr auto operator~(EnumFlags flags) -> EnumFlags {
		flags.m_bits = ~flags.m_bits;
		return flags;
	}

	friend constexpr auto operator|(EnumFlags a, EnumFlags const b) {
		a |= b;
		return a;
	}

	friend constexpr auto operator&(EnumFlags a, EnumFlags const b) {
		a &= b;
		return a;
	}

	auto operator==(EnumFlags const&) const -> bool = default;

  private:
	std::underlying_type_t<E> m_bits{};
};
} // namespace klib
