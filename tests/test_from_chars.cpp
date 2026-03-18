#include "klib/string/from_chars.hpp"
#include "klib/unit_test/unit_test.hpp"
#include <cmath>
#include <cstdint>

namespace {
using namespace klib;

TEST(from_chars_single) {
	auto i = int{};
	EXPECT(FromChars{.text = "42"}(i));
	EXPECT(i == 42);

	auto f = float{};
	EXPECT(FromChars{.text = "3.14"}(f));
	EXPECT(std::abs(f - 3.14) < 0.01f);
}

TEST(from_chars_parse) {
	auto i = int{};
	auto fc = FromChars{.text = "1,234.56"};
	EXPECT(fc(i));
	EXPECT(i == 1);
	ASSERT(fc.advance_if(','));

	EXPECT(fc(i));
	EXPECT(i == 234);
	ASSERT(fc.advance_if('.'));

	EXPECT(fc(i));
	EXPECT(i == 56);
	EXPECT(fc.text.empty());
}

TEST(from_chars_advance) {
	auto fc = FromChars{.text = "abc@xyz.com"};
	EXPECT(fc.advance_if_any("paq"));
	EXPECT(fc.advance_if_all("bc"));
	EXPECT(fc.advance_if('@'));
	EXPECT(fc.advance_if_all("xyz"));
	EXPECT(fc.advance_if('.'));
	EXPECT(fc.advance_if_all("com"));
}

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
