#include "klib/string/lerp_expr.hpp"
#include "klib/unit_test/unit_test.hpp"
#include <format>
#include <unordered_map>

namespace {
using namespace klib;

static_assert([] {
	using Scanner = LerpExprScanner;
	using Token = LerpExprToken;

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

TEST_CASE(lerp_expr) {
	auto map = std::unordered_map<std::string_view, std::string_view>{};
	map["level"] = "I";
	map["tag"] = "LogTag";
	map["message"] = "log message";
	map["timestamp"] = "01:23:45";
	map["source_file"] = __FILE__;
	map["source_line"] = "42";
	auto const format_value = [&](std::string& out, std::string_view const identifier) {
		auto const it = map.find(identifier);
		if (it == map.end()) { return; }
		out.append(it->second);
	};

	auto const formatted = klib::lerp_expr("[{level}] [{tag}] {message} [{timestamp}] [{source_file}:{source_line}]", format_value);
	auto const expected =
		std::format("[{}] [{}] {} [{}] [{}:{}]", map["level"], map["tag"], map["message"], map["timestamp"], map["source_file"], map["source_line"]);
	EXPECT(formatted == expected);
}
} // namespace
