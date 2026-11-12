#pragma once
#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>

namespace klib::lerp_expr {
struct Token {
	struct Highlight;

	enum class Type : std::int8_t { String, Identifier };

	Type type{};
	std::string_view lexeme{};
	std::size_t start_index{};
};

struct Token::Highlight {
	void format_to(std::string& out, Token const& token, std::string_view input_line) const;
	[[nodiscard]] auto format(Token const& token, std::string_view input_line) const -> std::string;

	std::string_view prefix{" | "};
	char caret{'^'};
};
} // namespace klib::lerp_expr
