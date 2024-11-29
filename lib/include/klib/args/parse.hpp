#pragma once
#include <klib/args/arg.hpp>
#include <klib/args/parse_info.hpp>
#include <klib/args/parse_result.hpp>
#include <klib/build_version.hpp>

namespace klib::args {
[[nodiscard]] auto parse_args(ParseInfo const& info, std::span<Arg const> args, int argc, char const* const* argv) -> ParseResult;

[[nodiscard]] inline auto parse_args(std::span<Arg const> args, int argc, char const* const* argv) -> ParseResult { return parse_args({}, args, argc, argv); }
} // namespace klib::args
