#include "klib/string/interpolator.hpp"
#include "klib/unit_test/unit_test.hpp"
#include <format>
#include <string_view>
#include <unordered_map>

namespace {
using namespace klib;

struct Interpolator : StringInterpolator {
	void format_value_to(std::string& out, std::string_view const identifier) const final {
		auto const it = map.find(identifier);
		if (it == map.end()) { return; }
		out.append(it->second);
	}

	std::unordered_map<std::string_view, std::string_view> map{};
};

TEST_CASE(string_interpolator) {
	auto interpolator = Interpolator{};
	interpolator.map["level"] = "I";
	interpolator.map["tag"] = "LogTag";
	interpolator.map["message"] = "log message";
	interpolator.map["timestamp"] = "01:23:45";
	interpolator.map["source_file"] = __FILE__;
	interpolator.map["source_line"] = "42";
	auto const formatted = interpolator.interpolate("[{level}] [{tag}] {message} [{timestamp}] [{source_file}:{source_line}]");
	auto const expected = std::format("[{}] [{}] {} [{}] [{}:{}]", interpolator.map["level"], interpolator.map["tag"], interpolator.map["message"],
									  interpolator.map["timestamp"], interpolator.map["source_file"], interpolator.map["source_line"]);
	EXPECT(formatted == expected);
}
} // namespace
