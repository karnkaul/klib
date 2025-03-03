#pragma once
#include <klib/args/printer.hpp>
#include <klib/enum_ops.hpp>
#include <cstdint>
#include <string_view>

namespace klib {
namespace args {
enum class ParseFlag : std::int8_t;
} // namespace args

template <>
inline constexpr auto enable_enum_ops_v<args::ParseFlag> = true;
} // namespace klib

namespace klib::args {
enum class ParseFlag : std::int8_t {
	None = 0,
	/// \brief Omit printing default values of optional positional args.
	OmitDefaultValues = 1 << 0,
};

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
