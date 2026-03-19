#pragma once
#include "klib/string/format_parser.hpp"
#include <compare>
#include <cstdint>
#include <format>

namespace klib {
struct Version {
	std::int64_t major{};
	std::int64_t minor{};
	std::int64_t patch{};

	auto operator<=>(Version const&) const -> std::strong_ordering = default;
};

[[nodiscard]] auto to_version(std::string_view text) -> Version;
} // namespace klib

template <>
struct std::formatter<klib::Version> : klib::FormatParser {
	static auto format(klib::Version const& version, format_context& fc) -> format_context::iterator;
};
