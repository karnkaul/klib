#pragma once
#include <klib/constants.hpp>
#include <compare>

namespace klib {
template <std::uint64_t Factor = 1>
struct ByteCount {
	static constexpr std::uint64_t factor_v = Factor;

	constexpr explicit ByteCount(std::int64_t const value = {}) : m_value(value) {}

	template <std::uint64_t OtherFactor>
	constexpr ByteCount(ByteCount<OtherFactor> const bc) : m_value((bc.count() * bc.factor_v) / factor_v) {}

	constexpr auto operator+=(ByteCount const bc) -> ByteCount& {
		m_value += bc.m_value;
		return *this;
	}

	constexpr auto operator-=(ByteCount const bc) -> ByteCount& {
		m_value -= bc.m_value;
		return *this;
	}

	constexpr auto operator*=(ByteCount const bc) -> ByteCount& {
		m_value *= bc.m_value;
		return *this;
	}

	constexpr auto operator/=(std::int64_t const divisor) -> ByteCount& {
		m_value /= divisor;
		return *this;
	}

	[[nodiscard]] constexpr auto count() const -> std::int64_t { return m_value; }

  private:
	std::int64_t m_value{};
};

template <std::uint64_t FactorL, std::uint64_t FactorR>
constexpr auto operator<=>(ByteCount<FactorL> const a, ByteCount<FactorR> const b) -> std::strong_ordering {
	return a.count() * a.factor_v <=> b.count() * b.factor_v;
}

template <std::uint64_t FactorL, std::uint64_t FactorR>
constexpr auto operator==(ByteCount<FactorL> const a, ByteCount<FactorR> const b) -> bool {
	return a.count() * a.factor_v == b.count() * b.factor_v;
}

template <std::uint64_t Factor>
constexpr auto operator+(ByteCount<Factor> const a, ByteCount<Factor> const b) -> ByteCount<Factor> {
	auto ret = a;
	ret += b;
	return ret;
}

template <std::uint64_t Factor>
constexpr auto operator-(ByteCount<Factor> const a, ByteCount<Factor> const b) -> ByteCount<Factor> {
	auto ret = a;
	ret -= b;
	return ret;
}

template <std::uint64_t Factor>
constexpr auto operator*(ByteCount<Factor> const a, ByteCount<Factor> const b) -> ByteCount<Factor> {
	auto ret = a;
	ret *= b;
	return ret;
}

template <std::uint64_t Factor>
constexpr auto operator/(ByteCount<Factor> const a, std::uint64_t const divisor) -> ByteCount<Factor> {
	auto ret = a;
	ret /= divisor;
	return ret;
}

template <std::uint64_t Factor>
constexpr auto operator/(ByteCount<Factor> const a, ByteCount<Factor> const b) -> std::uint64_t {
	return a.count() / b.count();
}

using Bytes = ByteCount<1>;
using KibiBytes = ByteCount<kibi_v>;
using MebiBytes = ByteCount<mebi_v>;
using GibiBytes = ByteCount<gibi_v>;
using TebiBytes = ByteCount<tebi_v>;
using KiloBytes = ByteCount<kilo_v>;
using MegaBytes = ByteCount<mega_v>;
using GigaBytes = ByteCount<giga_v>;
using TeraBytes = ByteCount<tera_v>;

namespace literals {
constexpr auto operator""_B(unsigned long long value) { return Bytes{std::int64_t(value)}; }
constexpr auto operator""_KiB(unsigned long long value) { return KibiBytes{std::int64_t(value)}; }
constexpr auto operator""_MiB(unsigned long long value) { return MebiBytes{std::int64_t(value)}; }
constexpr auto operator""_GiB(unsigned long long value) { return GibiBytes{std::int64_t(value)}; }
constexpr auto operator""_TiB(unsigned long long value) { return TebiBytes{std::int64_t(value)}; }
constexpr auto operator""_KB(unsigned long long value) { return KiloBytes{std::int64_t(value)}; }
constexpr auto operator""_MB(unsigned long long value) { return MegaBytes{std::int64_t(value)}; }
constexpr auto operator""_GB(unsigned long long value) { return GigaBytes{std::int64_t(value)}; }
constexpr auto operator""_TB(unsigned long long value) { return TeraBytes{std::int64_t(value)}; }
} // namespace literals
} // namespace klib
