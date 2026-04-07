#pragma once
#include "klib/debug/assert.hpp"
#include "klib/lerp_expr/token.hpp"
#include <cstddef>

namespace klib::lerp_expr {
template <typename FuncT>
concept PerTokenT = requires(FuncT func, Token const& token) {
	{ func(token) };
};

class Scanner {
  public:
	static constexpr std::string_view enclose_v = "{}";

	using Type = Token::Type;

	explicit constexpr Scanner(std::string_view const text) : m_text(text) {}

	constexpr auto scan_next(Token& out) -> bool {
		if (m_index >= m_text.size()) { return false; }

		auto const i = std::min(m_text.find(enclose_v[0], m_index), m_text.size());
		if (i == m_index) {
			out = to_identifier();
			return true;
		}

		out = to_token(Type::String, i - m_index);
		return true;
	}

  private:
	[[nodiscard]] constexpr auto to_identifier() -> Token {
		KLIB_ASSERT(m_index < m_text.size() && m_text[m_index] == enclose_v[0]);
		++m_index;

		auto const i = m_text.find(enclose_v[1], m_index);
		if (i == std::string_view::npos) { return to_token(Type::Identifier, m_text.size()); }

		auto const ret = to_token(Type::Identifier, i - m_index);
		++m_index;

		return ret;
	}

	[[nodiscard]] constexpr auto to_token(Type const type, std::size_t length) -> Token {
		length = std::min(length, m_text.size() - m_index);
		auto const ret = Token{
			.type = type,
			.lexeme = m_text.substr(m_index, length),
			.start_index = m_index,
		};
		m_index += length;
		return ret;
	}

	std::string_view m_text{};
	std::size_t m_index{};
};

template <PerTokenT Func>
constexpr void tokenize(std::string_view const text, Func per_token) {
	auto scanner = Scanner{text};
	auto token = Token{};
	while (scanner.scan_next(token)) { per_token(token); }
}
} // namespace klib::lerp_expr
