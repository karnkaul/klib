#include "klib/from_chars.hpp"
#include "klib/unit_test.hpp"
#include <cmath>

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
} // namespace
