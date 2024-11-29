#pragma once
#include <klib/arg.hpp>
#include <klib/arg_parse_info.hpp>
#include <klib/arg_parse_result.hpp>
#include <klib/build_version.hpp>

namespace klib {
[[nodiscard]] auto parse_args(ArgParseInfo const& info, std::span<Arg const> args, int argc, char const* const* argv) -> ArgParseResult;

[[nodiscard]] inline auto parse_args(std::span<Arg const> args, int argc, char const* const* argv) -> ArgParseResult {
	return parse_args({}, args, argc, argv);
}
} // namespace klib
