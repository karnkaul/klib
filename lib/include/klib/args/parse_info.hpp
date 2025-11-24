#pragma once
#include "klib/enum_ops.hpp"
#include <klib/args/printer.hpp>
#include <cstdint>
#include <string_view>

namespace klib::args {
enum class ParseFlag : std::int8_t {
	None = 0,
	/// \brief Omit printing default values of optional positional args.
	OmitDefaultValues = 1 << 0,
};
constexpr auto enable_enum_bitops(ParseFlag /*unused*/) -> bool { return true; }

struct ParseInfo {
	/// \brief One liner app description.
	std::string_view help_text{};
	/// \brief Version text.
	std::string_view version{};
	/// \brief Help text epilogue.
	std::string_view epilogue{};
	/// \brief Custom printer.
	IPrinter* printer{nullptr};

	ParseFlag flags{ParseFlag::None};
};

struct ParseStringInfo {
	/// \brief Custom printer.
	IPrinter* printer{nullptr};

	ParseFlag flags{ParseFlag::None};
};
} // namespace klib::args
