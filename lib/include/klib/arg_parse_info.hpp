#pragma once
#include <string_view>

namespace klib {
/// \brief Application info.
struct ArgParseInfo {
	/// \brief One liner app description.
	std::string_view help_text{};
	/// \brief Version text.
	std::string_view version{};
	/// \brief Help text epilogue.
	std::string_view epilogue{};
};
} // namespace klib
