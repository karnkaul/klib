#pragma once
#include "klib/debug/assert.hpp"
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <string_view>

namespace klib::detail {
struct InterpolateToken {
	enum class Type : std::int8_t { String, Identifier };

	Type type{};
	std::string_view lexeme{};
};

class InterpolateScanner {
  public:
	static constexpr std::string_view enclose_v = "{}";

	using Token = InterpolateToken;
	using Type = Token::Type;

	explicit constexpr InterpolateScanner(std::string_view const text) : m_remain(text) {}

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
} // namespace klib::detail
