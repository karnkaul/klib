#pragma once
#include <klib/args/arg.hpp>
#include <klib/args/parse_info.hpp>
#include <klib/args/parse_result.hpp>
#include <klib/args/printer.hpp>

namespace klib::args {
[[nodiscard]] auto parse_string(std::span<Arg const> args, std::string_view input, IPrinter* printer = nullptr) -> ParseResult;

[[nodiscard]] auto parse_main(ParseInfo const& info, std::span<Arg const> args, int argc, char const* const* argv) -> ParseResult;

[[nodiscard]] inline auto parse_main(std::span<Arg const> args, int argc, char const* const* argv) -> ParseResult { return parse_main({}, args, argc, argv); }
} // namespace klib::args
