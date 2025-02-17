#pragma once
#include <klib/args/printer.hpp>
#include <string_view>

namespace klib::args {
struct ParseInfo {
	/// \brief One liner app description.
	std::string_view help_text{};
	/// \brief Version text.
	std::string_view version{};
	/// \brief Help text epilogue.
	std::string_view epilogue{};
	/// \brief Custom printer.
	IPrinter* printer{nullptr};
};
} // namespace klib::args
