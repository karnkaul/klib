#include "klib/lerp_expr/interpolate.hpp"
#include "klib/unit_test/unit_test.hpp"
#include <format>
#include <string_view>
#include <unordered_map>

namespace {
using namespace klib::lerp_expr;

static_assert([] {
	constexpr auto text = std::string_view{"Abc - {foo} bar.{ext}"};
	auto scanner = Scanner{text};
	auto token = Token{};

	if (!scanner.scan_next(token)) { return false; }
	if (token.type != Token::Type::String) { return false; }
	if (token.lexeme != "Abc - ") { return false; }

	if (!scanner.scan_next(token)) { return false; }
	if (token.type != Token::Type::Identifier) { return false; }
	if (token.lexeme != "foo") { return false; }

	if (!scanner.scan_next(token)) { return false; }
	if (token.type != Token::Type::String) { return false; }
	if (token.lexeme != " bar.") { return false; }

	if (!scanner.scan_next(token)) { return false; }
	if (token.type != Token::Type::Identifier) { return false; }
	if (token.lexeme != "ext") { return false; }

	return !scanner.scan_next(token);
}());

TEST_CASE(lerp_expr_interpolate) {
	auto map = std::unordered_map<std::string_view, std::string_view>{};
	map["level"] = "I";
	map["tag"] = "LogTag";
	map["message"] = "log message";
	map["timestamp"] = "01:23:45";
	map["source_file"] = __FILE__;
	map["source_line"] = "42";
	auto const format_identifier = [&](std::string& out, Token const& identifier) {
		auto const it = map.find(identifier.lexeme);
		if (it == map.end()) { return; }
		out.append(it->second);
	};
	static_assert(FormatIdentifierT<decltype(format_identifier)>);

	auto const formatted = interpolate("[{level}] [{tag}] {message} [{timestamp}] [{source_file}:{source_line}]", format_identifier);
	auto const expected =
		std::format("[{}] [{}] {} [{}] [{}:{}]", map["level"], map["tag"], map["message"], map["timestamp"], map["source_file"], map["source_line"]);
	EXPECT(formatted == expected);
}

TEST_CASE(lerp_expr_highlight) {
	static constexpr std::string_view text_v{"foo bar fubar"};
	static constexpr auto token_v = Token{.lexeme = "bar", .start_index = text_v.find("bar")};
	auto highlight = Token::Highlight{};
	auto res = highlight.format(token_v, text_v);
	std::string_view expected = R"( | foo bar fubar
 |     ^^^)";
	EXPECT(res == expected);

	res = "Error";
	highlight.caret = '~';
	highlight.format_to(res, token_v, text_v);
	expected = R"(Error
 | foo bar fubar
 |     ~~~)";
	EXPECT(res == expected);
}
} // namespace
