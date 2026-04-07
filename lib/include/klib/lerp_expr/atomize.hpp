#pragma once
#include "klib/lerp_expr/scanner.hpp"
#include <variant>
#include <vector>

namespace klib::lerp_expr {
template <typename ProjT, typename ValueT>
concept ProjectIdentifierT = requires(ProjT proj, std::string_view id) {
	{ proj(id) } -> std::convertible_to<ValueT>;
};

template <typename ValueT>
struct Atom {
	std::variant<std::string, ValueT> payload{};
	Token token{};
};

template <typename ValueT, ProjectIdentifierT<ValueT> ProjT>
constexpr void atomize_to(std::vector<Atom<ValueT>>& out_atoms, std::string_view const text, ProjT proj) {
	auto const per_token = [&](Token const& token) {
		switch (token.type) {
#if defined(__GNUG__) && !defined(__clang__)
// GCC warns about std::string internals being uninitialized, despite this branch not using that variant type.
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmaybe-uninitialized"
#endif
		case Token::Type::Identifier: out_atoms.push_back(Atom<ValueT>{.payload = proj(token.lexeme), .token = token}); break;
#if defined(__GNUG__) && !defined(__clang__)
#pragma GCC diagnostic pop
#endif
		case Token::Type::String: out_atoms.push_back(Atom<ValueT>{.payload = std::string{token.lexeme}, .token = token}); break;
		default: break;
		}
	};
	tokenize(text, per_token);
}

template <typename ValueT, ProjectIdentifierT<ValueT> ProjT>
[[nodiscard]] auto atomize(std::string_view const text, ProjT proj) -> std::vector<Atom<ValueT>> {
	auto ret = std::vector<Atom<ValueT>>{};
	atomize_to(ret, text, std::move(proj));
	return ret;
}
} // namespace klib::lerp_expr
