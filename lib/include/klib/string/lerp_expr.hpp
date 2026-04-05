#pragma once
#include "klib/debug/assert.hpp"
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <string_view>
#include <variant>
#include <vector>

namespace klib {
struct LerpExprToken {
	enum class Type : std::int8_t { String, Identifier };

	Type type{};
	std::string_view lexeme{};
};

class LerpExprScanner {
  public:
	static constexpr std::string_view enclose_v = "{}";

	using Token = LerpExprToken;
	using Type = Token::Type;

	explicit constexpr LerpExprScanner(std::string_view const text) : m_remain(text) {}

	constexpr auto scan_next(Token& out) -> bool {
		if (m_remain.empty()) { return false; }

		auto const i = std::min(m_remain.find(enclose_v[0]), m_remain.size());
		if (i == 0) {
			out = to_identifier();
			return true;
		}

		out = to_string_token(i);
		return true;
	}

  private:
	[[nodiscard]] constexpr auto to_string_token(std::size_t const length) -> Token {
		auto const ret = Token{.type = Type::String, .lexeme = m_remain.substr(0, length)};
		m_remain.remove_prefix(length);
		return ret;
	}

	[[nodiscard]] constexpr auto to_identifier() -> Token {
		KLIB_ASSERT(!m_remain.empty() && m_remain.front() == enclose_v[0]);
		m_remain.remove_prefix(1);

		auto const i = m_remain.find(enclose_v[1]);
		if (i == std::string_view::npos) {
			auto const ret = Token{.type = Type::Identifier, .lexeme = m_remain};
			m_remain = {};
			return ret;
		}

		auto const ret = Token{.type = Type::Identifier, .lexeme = m_remain.substr(0, i)};
		m_remain.remove_prefix(i + 1);
		return ret;
	}

	std::string_view m_remain{};
};

template <typename FuncT>
concept PerLerpExprTokenT = requires(FuncT proj, LerpExprToken const& token) {
	{ proj(token) };
};

template <typename FuncT>
concept FormatLerpExprTokenT = requires(FuncT func, std::string& out, std::string_view id) {
	{ func(out, id) };
};

template <typename ValueT>
using LerpExprAtom = std::variant<std::string, ValueT>;

template <typename ProjT, typename ValueT>
concept ProjLerpExprTokenT = requires(ProjT proj, std::string_view id) {
	{ proj(id) } -> std::convertible_to<ValueT>;
};

template <PerLerpExprTokenT Func>
void tokenize_lerp_expr(std::string_view const text, Func per_token) {
	auto scanner = LerpExprScanner{text};
	auto token = LerpExprToken{};
	while (scanner.scan_next(token)) { per_token(token); }
}

template <FormatLerpExprTokenT FuncT>
void lerp_expr_to(std::string& out, std::string_view input, FuncT format_value) {
	auto const per_token = [&](LerpExprToken const& token) {
		switch (token.type) {
		case LerpExprToken::Type::String: out.append(token.lexeme); break;
		case LerpExprToken::Type::Identifier: format_value(out, token.lexeme); break;
		default: break;
		}
	};
	tokenize_lerp_expr(input, per_token);
}

template <FormatLerpExprTokenT FuncT>
[[nodiscard]] auto lerp_expr(std::string_view input, FuncT format_value) -> std::string {
	auto ret = std::string{};
	lerp_expr_to(ret, input, std::move(format_value));
	return ret;
}

template <typename ValueT, ProjLerpExprTokenT<ValueT> ProjT>
void atomize_lerp_expr_to(std::vector<LerpExprAtom<ValueT>>& out_atoms, std::string_view const text, ProjT proj) {
	auto const per_token = [&](LerpExprToken const& token) {
		switch (token.type) {
		case LerpExprToken::Type::Identifier: out_atoms.emplace_back(proj(token.lexeme)); break;
		case LerpExprToken::Type::String: out_atoms.emplace_back(std::string{token.lexeme}); break;
		default: break;
		}
	};
	tokenize_lerp_expr(text, per_token);
}
} // namespace klib
