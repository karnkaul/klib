#pragma once
#include <klib/args/arg.hpp>
#include <klib/args/parse_info.hpp>
#include <klib/args/parse_result.hpp>
#include <klib/args/printer.hpp>

namespace klib::args {
[[nodiscard]] auto parse_string(ParseStringInfo const& info, std::span<Arg const> args, std::string_view input) -> ParseResult;

[[nodiscard]] auto parse_main(ParseInfo const& info, std::span<Arg const> args, int argc, char const* const* argv) -> ParseResult;

[[nodiscard]] inline auto parse_main(std::span<Arg const> args, int argc, char const* const* argv) -> ParseResult { return parse_main({}, args, argc, argv); }

struct HelpString {
	std::string_view exe_name{};
	std::string_view help_text{};
	std::string_view version{};
	std::string_view epilogue{};
	ParseFlag flags{};

	[[nodiscard]] auto operator()(std::span<Arg const> args) const -> std::string;
};

struct CmdHelpString {
	std::string_view exe_name{};
	std::string_view cmd_name{};
	std::string_view help_text{};
	ParseFlag flags{};

	[[nodiscard]] auto operator()(std::span<Arg const> args) const -> std::string;
};

struct UsageString {
	std::string_view exe_name{};
	ParseFlag flags{};

	[[nodiscard]] auto operator()(std::span<Arg const> args) const -> std::string;
};

struct CmdUsageString {
	std::string_view exe_name{};
	std::string_view cmd_name{};
	ParseFlag flags{};

	[[nodiscard]] auto operator()(std::span<Arg const> args) const -> std::string;
};
} // namespace klib::args
