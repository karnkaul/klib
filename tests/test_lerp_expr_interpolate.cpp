#include "klib/lerp_expr/interpolate.hpp"
#include "klib/unit_test/unit_test.hpp"
#include <format>
#include <unordered_map>

namespace {
using namespace klib::lerp_expr;

TEST_CASE(lerp_expr_interpolate) {
	auto map = std::unordered_map<std::string_view, std::string_view>{};
	map["level"] = "I";
	map["tag"] = "LogTag";
	map["message"] = "log message";
	map["timestamp"] = "01:23:45";
	map["source_file"] = __FILE__;
	map["source_line"] = "42";
	auto const format_identifier = [&](std::string& out, std::string_view const identifier) {
		auto const it = map.find(identifier);
		if (it == map.end()) { return; }
		out.append(it->second);
	};
	static_assert(FormatIdentifierT<decltype(format_identifier)>);

	auto const formatted = interpolate("[{level}] [{tag}] {message} [{timestamp}] [{source_file}:{source_line}]", format_identifier);
	auto const expected =
		std::format("[{}] [{}] {} [{}] [{}:{}]", map["level"], map["tag"], map["message"], map["timestamp"], map["source_file"], map["source_line"]);
	EXPECT(formatted == expected);
}
} // namespace
