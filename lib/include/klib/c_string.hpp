#pragma once
#include <string>

namespace klib {
/// \brief Wrapper over a C string.
/// Never points to nullptr.
class CString {
  public:
	CString() = default;

	CString(std::nullptr_t) = delete;

	template <std::size_t N>
	// NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays,modernize-avoid-c-arrays)
	explicit(false) constexpr CString(char const (&str)[N]) : m_str(str, N) {}

	explicit(false) constexpr CString(char const* str) : m_str(str ? str : "") {}

	template <std::same_as<std::string> T>
	explicit(false) constexpr CString(T const& str) : m_str(str) {}

	[[nodiscard]] constexpr auto as_view() const -> std::string_view { return m_str; }
	[[nodiscard]] constexpr auto c_str() const -> char const* { return m_str.data(); }

	auto operator<=>(CString const&) const = default;

  private:
	// NOLINTNEXTLINE(readability-redundant-string-init)
	std::string_view m_str{""};
};
} // namespace klib
