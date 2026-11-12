import klib.core.string;

#include "klib/unit_test/unit_test.hpp"

namespace {
TEST_CASE(glob_empty) {
	auto const glob = klib::Glob{};
	EXPECT(!glob.is_exact_match("foo"));
}

TEST_CASE(glob_wildcards) {
	auto glob = klib::Glob::create("foo*");
	ASSERT(glob.has_value());
	EXPECT(glob->is_exact_match("foo"));
	EXPECT(glob->is_exact_match("foobar"));
	EXPECT(glob->is_exact_match("foo bar"));
	EXPECT(!glob->is_exact_match("fubar"));
	EXPECT(glob->is_partial_match("is foo bar"));
	EXPECT(!glob->is_partial_match("is fubar"));

	glob = klib::Glob::create("c?t");
	ASSERT(glob.has_value());
	EXPECT(glob->is_exact_match("cat"));
	EXPECT(glob->is_exact_match("cut"));
	EXPECT(!glob->is_exact_match("car"));
	EXPECT(glob->is_partial_match("is cat"));

	glob = klib::Glob::create("*debug*.log");
	ASSERT(glob.has_value());
	EXPECT(glob->is_exact_match("debug.log"));
	EXPECT(glob->is_exact_match("debug_1.log"));
	EXPECT(glob->is_exact_match("debug.klib.log"));
	EXPECT(glob->is_exact_match("klib.debug.log"));
}

TEST_CASE(glob_brackets) {
	auto glob = klib::Glob::create("[Cc]at");
	ASSERT(glob.has_value());
	EXPECT(glob->is_exact_match("cat"));
	EXPECT(glob->is_exact_match("Cat"));
	EXPECT(!glob->is_exact_match("car"));
	EXPECT(glob->is_partial_match("is Cat"));

	glob = klib::Glob::create("?.[1-9][0-9]");
	ASSERT(glob.has_value());
	EXPECT(glob->is_exact_match("x.42"));
	EXPECT(!glob->is_exact_match("x-42"));
	EXPECT(!glob->is_exact_match("x.04"));
}
} // namespace
