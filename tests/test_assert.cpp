import klib.core;

#include "klib/unit_test/unit_test.hpp"

namespace {
TEST_CASE(assert) {
	[[maybe_unused]] static constexpr int value_v{42};
	auto thrown = false;
	try {
		klib_assert_debug(value_v == -5);
	} catch (klib::assertion::Failure const& /*e*/) {
		thrown = true;
	}
	EXPECT(thrown == klib::debug_v);

	klib::assertion::set_fail_action(klib::assertion::FailAction::None);
	thrown = false;
	try {
		klib_assert(value_v == -5);
	} catch (klib::assertion::Failure const& /*e*/) {
		thrown = true;
	}
	EXPECT(thrown == false);
}
} // namespace
