#include "klib/unit_test.hpp"
#include "klib/version_str.hpp"

namespace {
using namespace klib;

TEST(version_to_str) {
	static constexpr std::string_view expected_v{"v1.23.456"};
	auto const version = std::format("{}", Version{.major = 1, .minor = 23, .patch = 456});
	EXPECT(version == expected_v);
}

TEST(version_from_str) {
	static constexpr std::string_view input_v{"v1.23.456"};
	static constexpr auto expected_v = Version{.major = 1, .minor = 23, .patch = 456};
	auto const version = to_version(input_v);
	EXPECT(version == expected_v);
}
} // namespace
