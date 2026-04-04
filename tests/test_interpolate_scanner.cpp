#include "detail/string/interpolate_impl.hpp"
#include <string_view>

namespace {
using namespace klib;

static_assert([] {
	using Scanner = detail::InterpolateScanner;
	using Token = detail::InterpolateToken;

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
} // namespace
