#pragma once
#include <array>
#include <concepts>
#include <stdexcept>

namespace klib {
template <std::default_initializable Type, std::size_t Capacity>
class FlexArray {
  public:
	using value_type = Type;

	static constexpr std::size_t capacity_v = Capacity;

	using iterator = Type*;
	using const_iterator = Type const*;
	using reverse_iterator = std::reverse_iterator<iterator>;
	using const_reverse_iterator = std::reverse_iterator<const_iterator>;

	FlexArray() = default;

	template <std::convertible_to<Type>... T>
		requires(sizeof...(T) <= capacity_v)
	constexpr FlexArray(T... t) : m_arr{std::move(t)...}, m_size(sizeof...(T)) {}

	constexpr void push_back(Type t) noexcept(false) {
		if (is_full()) { throw std::runtime_error{"push_back() called on full FlexArray"}; }
		m_arr[m_size++] = std::move(t);
	}

	constexpr auto try_push_back(Type t) -> bool {
		if (m_size == capacity_v) { return false; }
		m_arr[m_size++] = std::move(t);
		return true;
	}

	constexpr auto pop_back() noexcept(false) -> Type {
		if (is_empty()) { throw std::runtime_error{"pop_back() called on empty FlexArray"}; }
		return std::move(m_arr[m_size-- - 1]);
	}

	constexpr auto try_pop_back(Type& out) noexcept(false) -> bool {
		if (is_empty()) { return false; }
		out = std::move(m_arr[m_size-- - 1]);
		return true;
	}

	template <std::invocable<Type&> Pred>
	constexpr void erase_unordered_if(Pred pred) {
		for (auto& t : *this) {
			if (pred(t)) {
				if (m_size > 1) {
					using std::swap;
					swap(t, back());
				}
				pop_back();
				return;
			}
		}
	}

	[[nodiscard]] constexpr auto size() const -> std::size_t { return m_size; }
	[[nodiscard]] constexpr auto is_empty() const -> bool { return m_size == 0; }
	[[nodiscard]] constexpr auto empty() const -> bool { return is_empty(); }
	[[nodiscard]] constexpr auto is_full() const -> bool { return m_size >= capacity_v; }

	[[nodiscard]] constexpr auto at(std::size_t const index) const -> Type const& { return m_arr.at(index); }
	[[nodiscard]] constexpr auto at(std::size_t const index) -> Type& { return m_arr.at(index); }

	[[nodiscard]] constexpr auto front() const -> Type const& { return m_arr.front(); }
	[[nodiscard]] constexpr auto front() -> Type& { return m_arr.front(); }

	[[nodiscard]] constexpr auto back() const -> Type const& { return m_arr.at(m_size - 1); }
	[[nodiscard]] constexpr auto back() -> Type& { return m_arr.at(m_size - 1); }

	[[nodiscard]] constexpr auto data() const -> Type const* { return m_arr.data(); }
	[[nodiscard]] constexpr auto data() -> Type* { return m_arr.data(); }

	[[nodiscard]] constexpr auto cbegin() const -> const_iterator { return m_arr.data(); }
	[[nodiscard]] constexpr auto cend() const -> const_iterator { return m_arr.data() + m_size; }
	[[nodiscard]] constexpr auto begin() const -> const_iterator { return cbegin(); }
	[[nodiscard]] constexpr auto end() const -> const_iterator { return cend(); }

	[[nodiscard]] constexpr auto begin() -> iterator { return m_arr.data(); }
	[[nodiscard]] constexpr auto end() -> iterator { return m_arr.data() + m_size; }

	[[nodiscard]] constexpr auto crbegin() const -> const_iterator { return std::reverse_iterator<const_iterator>{end()}; }
	[[nodiscard]] constexpr auto rbegin() const -> const_iterator { return crbegin(); }
	[[nodiscard]] constexpr auto crend() const -> const_iterator { return std::reverse_iterator<const_iterator>{begin()}; }
	[[nodiscard]] constexpr auto rend() const -> const_iterator { return crend(); }

	[[nodiscard]] constexpr auto rbegin() -> iterator { return std::reverse_iterator<iterator>{end()}; }
	[[nodiscard]] constexpr auto rend() -> iterator { return std::reverse_iterator<iterator>{begin()}; }

	[[nodiscard]] constexpr auto operator[](std::size_t const index) const -> Type const& { return m_arr[index]; }
	[[nodiscard]] constexpr auto operator[](std::size_t const index) -> Type& { return m_arr[index]; }

  private:
	std::array<Type, capacity_v> m_arr{};
	std::size_t m_size{};
};
} // namespace klib
