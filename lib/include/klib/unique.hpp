#pragma once
#include <utility>

namespace klib {
template <typename Type, typename Deleter>
class Unique {
  public:
	Unique(Unique const&) = delete;
	auto operator=(Unique const&) -> Unique& = delete;

	constexpr Unique(Type t = Type{}, Deleter deleter = Deleter{}) : m_t(std::move(t)), m_deleter(std::move(deleter)) {}

	constexpr Unique(Unique&& rhs) noexcept : m_t(std::exchange(rhs.m_t, Type{})), m_deleter(std::move(rhs.m_deleter)) {}

	constexpr auto operator=(Unique&& rhs) noexcept -> Unique& {
		using std::swap;
		if (this != &rhs) {
			swap(m_t, rhs.m_t);
			swap(m_deleter, rhs.m_deleter);
		}
		return *this;
	}

	constexpr ~Unique() { m_deleter(std::move(m_t)); }

  private:
	Type m_t;
	[[no_unique_address]] Deleter m_deleter;
};
} // namespace klib
