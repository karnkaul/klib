#pragma once
#include <klib/concepts.hpp>
#include <array>

namespace klib {
template <EnumT E, typename Type, std::size_t Size = std::size_t(E::COUNT_)>
class EnumArray : public std::array<Type, Size> {
  public:
	using BaseType = std::array<Type, Size>;

	EnumArray() = default;

	template <std::convertible_to<Type>... T>
		requires(sizeof...(T) == Size)
	constexpr EnumArray(T const&... t) : BaseType{t...} {}

	[[nodiscard]] constexpr auto at(E const e) const -> Type const& { return BaseType::at(std::size_t(e)); }
	[[nodiscard]] constexpr auto at(E const e) -> Type& { return BaseType::at(std::size_t(e)); }

	[[nodiscard]] constexpr auto operator[](E const e) const -> Type const& { return BaseType::operator[](std::size_t(e)); }
	[[nodiscard]] constexpr auto operator[](E const e) -> Type& { return BaseType::operator[](std::size_t(e)); }
};
} // namespace klib
