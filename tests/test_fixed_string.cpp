#include <klib/fixed_string.hpp>
#include <klib/unit_test.hpp>

namespace {
using namespace klib;

static_assert([] {
	auto str = FixedString<16>{};
	if (!str.is_empty()) { return false; }
	if (!str.as_view().empty()) { return false; }
	str = "hello world";
	if (str.as_view() != "hello world") { return false; }
	str = {};
	return str.is_empty();
}());

static_assert([] {
	auto str = FixedString<16>{"hello"};
	str += " world";
	return str.as_view() == "hello world";
}());

static_assert([] {
	auto str = FixedString<16>{"hello world"};
	str = str.substr(6, 2);
	return str.as_view() == "wo";
}());

TEST(fixed_string_format) {
	auto str = FixedString{"{}", 42};
	EXPECT(str.as_view() == "42");
	str += FixedString<8>{" {}", -1};
	EXPECT(str.as_view() == "42 -1");
}
} // namespace
