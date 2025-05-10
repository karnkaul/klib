#pragma once
#include <string_view>

namespace klib {
/// \brief Wrapper over a C string.
/// Never points to nullptr.
class CString {
  public:
	explicit(false) constexpr CString(char const* str = "") : m_str(str == nullptr ? "" : str) {}

	[[nodiscard]] constexpr auto c_str() const -> char const* { return as_view().data(); }
	[[nodiscard]] constexpr auto as_view() const -> std::string_view { return m_str; }

  private:
	std::string_view m_str;
};
} // namespace klib
