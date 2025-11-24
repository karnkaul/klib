#pragma once
#include "klib/macros.hpp"
#include "klib/noop.hpp"
#include <concepts>
#include <utility>

namespace klib {
template <typename Type>
concept UniquePayloadT = std::is_default_constructible_v<Type> && std::move_constructible<Type>;

template <UniquePayloadT Type>
	requires std::equality_comparable<Type>
struct UniqueIdentity {
	constexpr auto operator()(Type const& t) const noexcept -> bool { return t == Type{}; }
};

template <UniquePayloadT Type, typename Deleter = Noop<Type>, typename Id = UniqueIdentity<Type>>
class Unique {
  public:
	using value_type = Type;
	using deleter_type = Deleter;
	using id_type = Id;

	Unique(Unique const&) = delete;
	auto operator=(Unique const&) -> Unique& = delete;

	Unique() = default;

	constexpr Unique(Type t, Deleter deleter = Deleter{}, Id id = {}) : m_t(std::move(t)), m_deleter(std::move(deleter)), m_id(std::move(id)) {}

	constexpr Unique(Unique&& rhs) noexcept : m_t(std::exchange(rhs.m_t, Type{})), m_deleter(std::move(rhs.m_deleter)), m_id(std::move(rhs.m_id)) {}

	constexpr auto operator=(Unique&& rhs) noexcept -> Unique& {
		using std::swap;
		if (this != &rhs) {
			swap(m_t, rhs.m_t);
			swap(m_deleter, rhs.m_deleter);
			swap(m_id, rhs.m_id);
		}
		return *this;
	}

	constexpr ~Unique() {
		if (is_identity()) { return; }
		m_deleter(std::move(m_t));
	}

	[[nodiscard]] constexpr auto is_identity() const -> bool { return m_id(m_t); }

	[[nodiscard]] constexpr auto get() const -> Type const& { return m_t; }
	[[nodiscard]] constexpr auto get() -> Type& { return m_t; }

	[[nodiscard]] constexpr auto get_deleter() const -> deleter_type const& { return m_deleter; }
	[[nodiscard]] constexpr auto get_id() const -> id_type const& { return m_id; }

	constexpr operator Type const&() const { return get(); }
	constexpr operator Type&() const { return get(); }

  private:
	Type m_t;
	KLIB_NO_UNIQUE_ADDRESS Deleter m_deleter;
	KLIB_NO_UNIQUE_ADDRESS Id m_id;
};
} // namespace klib
