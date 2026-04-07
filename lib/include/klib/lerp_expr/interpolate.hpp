#pragma once
#include "klib/lerp_expr/scanner.hpp"

namespace klib::lerp_expr {
template <typename FuncT>
concept FormatIdentifierT = requires(FuncT func, std::string& out, std::string_view identifier) {
	{ func(out, identifier) };
};

template <FormatIdentifierT FuncT>
constexpr void interpolate_to(std::string& out, std::string_view const input, FuncT format_identifier) {
	auto const per_token = [&](Token const& token) {
		switch (token.type) {
		case Token::Type::String: out.append(token.lexeme); break;
		case Token::Type::Identifier: format_identifier(out, token.lexeme); break;
		default: break;
		}
	};
	tokenize(input, per_token);
}

template <FormatIdentifierT FuncT>
[[nodiscard]] constexpr auto interpolate(std::string_view const input, FuncT format_identifier) -> std::string {
	auto ret = std::string{};
	interpolate_to(ret, input, std::move(format_identifier));
	return ret;
}
} // namespace klib::lerp_expr
