#pragma once
#include <klib/assert.hpp>
#include <concepts>

namespace klib {
/// \brief Wrapper over a non-owning raw pointer.
/// Asserts on attempts to dereference if null.
template <typename Type>
class Ptr {
  public:
	Ptr() = default;

	explicit(false) constexpr Ptr(Type* t) : m_ptr(t) {}

	template <std::convertible_to<Type*> T>
	explicit(false) constexpr Ptr(T t) : m_ptr(static_cast<Type*>(t)) {}

	[[nodiscard]] constexpr auto get() const -> Type* { return m_ptr; }

	template <typename T>
		requires(std::derived_from<Type, T>)
	[[nodiscard]] explicit(false) constexpr operator T*() const {
		return get();
	}

	[[nodiscard]] constexpr auto operator*() const -> Type& {
		KLIB_ASSERT(m_ptr);
		return *m_ptr;
	}

	[[nodiscard]] constexpr auto operator->() const -> Type* {
		KLIB_ASSERT(m_ptr);
		return m_ptr;
	}

	[[nodiscard]] explicit(false) constexpr operator bool() const { return m_ptr != nullptr; }

	template <typename T>
	[[nodiscard]] constexpr auto operator==(Ptr<T> const& rhs) const -> bool {
		return get() == rhs.get();
	}

  private:
	Type* m_ptr{};
};
} // namespace klib
