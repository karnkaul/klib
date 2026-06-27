module;

#include "klib/debug.hpp"

export module klib.lerp_expr;

export import std;

import klib.core;

export namespace klib::lerp_expr {
struct Token {
	struct Highlight;

	enum class Type : std::int8_t { String, Identifier };

	Type type{};
	std::string_view lexeme{};
	std::size_t start_index{};
};

struct Token::Highlight {
	void format_to(std::string& out, Token const& token, std::string_view input_line) const {
		if (!out.empty()) { out.push_back('\n'); }
		std::format_to(std::back_inserter(out), "{}{}\n{}{:>{}}", prefix, input_line, prefix, ' ', token.start_index);
		for (auto i = 0uz; i < token.lexeme.size(); ++i) { out.push_back(caret); }
	}

	[[nodiscard]] auto format(Token const& token, std::string_view input_line) const -> std::string {
		auto ret = std::string{};
		format_to(ret, token, input_line);
		return ret;
	}

	std::string_view prefix{" | "};
	char caret{'^'};
};

template <typename FuncT>
concept PerTokenT = requires(FuncT func, Token const& token) {
	{ func(token) };
};

template <typename FuncT>
concept FormatIdentifierT = requires(FuncT func, std::string& out, Token const& identifier) {
	{ func(out, identifier) };
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

template <FormatIdentifierT FuncT>
constexpr void interpolate_to(std::string& out, std::string_view const input, FuncT format_identifier) {
	auto const per_token = [&](Token const& token) {
		switch (token.type) {
		case Token::Type::String: out.append(token.lexeme); break;
		case Token::Type::Identifier: format_identifier(out, token); break;
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
