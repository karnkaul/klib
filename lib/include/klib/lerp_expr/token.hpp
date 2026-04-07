#pragma once
#include <cstddef>
#include <cstdint>
#include <string_view>

namespace klib::lerp_expr {
struct Token {
	enum class Type : std::int8_t { String, Identifier };

	Type type{};
	std::string_view lexeme{};
	std::size_t start_index{};
};
} // namespace klib::lerp_expr
