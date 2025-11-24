#pragma once
#include "klib/format_parser.hpp"
#include "klib/version.hpp"
#include <format>

namespace klib {
[[nodiscard]] auto to_version(std::string_view text) -> Version;
} // namespace klib

template <>
struct std::formatter<klib::Version> : klib::FormatParser {
	static auto format(klib::Version const& version, std::format_context& fc) -> std::format_context::iterator;
};
