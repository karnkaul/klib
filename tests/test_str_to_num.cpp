#include "klib/str_to_num.hpp"
#include "klib/unit_test.hpp"
#include <cstdint>

namespace {
using namespace klib;

TEST(str_to_num_int) {
	static constexpr std::string_view forty_two_v{"42"};
	static constexpr std::string_view minus_3_v{"-3"};
	static constexpr std::string_view not_int_v{"foo"};

	auto const u32 = str_to_num(forty_two_v, 0u);
	EXPECT(u32 == 42);
	auto const i64 = str_to_num(minus_3_v, std::int64_t{-1});
	EXPECT(i64 == -3);
	auto const foo = str_to_num(not_int_v, -1);
	EXPECT(foo == -1);
}

TEST(str_to_num_float) {
	static constexpr std::string_view pi_v{"3.14"};
	static constexpr std::string_view not_float_v{"bar"};

	auto const pi = str_to_num(pi_v, -1.0f);
	EXPECT(std::abs(pi - 3.14f) < 0.01f);
	auto const bar = str_to_num(not_float_v, 0.0);
	EXPECT(bar == 0.0);
}
} // namespace
