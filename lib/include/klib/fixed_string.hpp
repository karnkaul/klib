#pragma once
#include <klib/str_buf.hpp>
#include <format>

namespace klib {
template <std::size_t MaxLength = 64>
class FixedString {
  public:
	FixedString() = default;

	template <std::convertible_to<std::string_view> T>
	explicit(false) constexpr FixedString(T const& text) : m_size(copy_to(m_buf, std::string_view{text})) {}

	template <typename... Args>
	explicit(false) FixedString(std::format_string<Args...> fmt, Args&&... args) {
		auto const [out, _] = std::format_to_n(m_buf.data(), std::iter_difference_t<char*>(MaxLength), fmt, std::forward<Args>(args)...);
		m_size = std::size_t(out - m_buf.data()); // NOLINT(cppcoreguidelines-prefer-member-initializer)
	}

	template <std::convertible_to<std::string_view> T>
	constexpr auto append(T const& rhs) -> FixedString& {
		auto const remain = std::span<char>{m_buf}.subspan(m_size);
		m_size += copy_to(remain, std::string_view{rhs});
		return *this;
	}

	[[nodiscard]] constexpr auto is_empty() const -> bool { return m_size == 0; }

	constexpr void clear() {
		m_buf = {};
		m_size = 0;
	}

	constexpr auto substr(std::size_t const pos, std::size_t const count = std::string_view::npos) const -> FixedString {
		return FixedString{as_view().substr(pos, count)};
	}

	[[nodiscard]] constexpr auto data() const -> char const* { return m_buf.data(); }
	[[nodiscard]] constexpr auto c_str() const -> char const* { return data(); }
	[[nodiscard]] constexpr auto as_view() const -> std::string_view { return std::string_view{data(), m_size}; }

	template <std::convertible_to<std::string_view> T>
	constexpr auto operator+=(T const& rhs) -> FixedString& {
		return append(rhs);
	}

	constexpr operator std::string_view() const { return as_view(); }

  private:
	StrBuf<MaxLength> m_buf{};
	std::size_t m_size{};
};
} // namespace klib
