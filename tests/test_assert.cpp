#include <klib/assert.hpp>
#include <klib/constants.hpp>
#include <klib/unit_test.hpp>

namespace {
TEST(assert) {
	[[maybe_unused]] static constexpr int value_v{42};
	auto thrown = false;
	try {
		KLIB_ASSERT(value_v == -5);
	} catch (klib::assertion::Failure const& /*e*/) { thrown = true; }
	EXPECT(thrown == klib::debug_v);

	klib::assertion::set_fail_action(klib::assertion::FailAction::None);
	thrown = false;
	try {
		KLIB_ASSERT(value_v == -5);
	} catch (klib::assertion::Failure const& /*e*/) { thrown = true; }
	EXPECT(thrown == false);
}
} // namespace
