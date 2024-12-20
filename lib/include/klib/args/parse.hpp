#pragma once
#include <klib/args/arg.hpp>
#include <klib/args/parse_info.hpp>
#include <klib/args/parse_result.hpp>

namespace klib::args {
[[nodiscard]] auto parse(ParseInfo const& info, std::span<Arg const> args, int argc, char const* const* argv) -> ParseResult;

[[nodiscard]] inline auto parse(std::span<Arg const> args, int argc, char const* const* argv) -> ParseResult { return parse({}, args, argc, argv); }
} // namespace klib::args
