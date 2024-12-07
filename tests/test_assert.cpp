#include <klib/assert.hpp>
#include <klib/constants.hpp>
#include <klib/unit_test.hpp>

namespace {
TEST(assert) {
	[[maybe_unused]] static constexpr int value_v{42};
	auto thrown = false;
	try {
		KLIB_ASSERT(value_v == -5);
	} catch (klib::AssertionFailure const& /*e*/) { thrown = true; }
	EXPECT(thrown == klib::debug_v);
}
} // namespace
