#include "klib/debug.hpp"
#include "klib/unit_test/unit_test.hpp"

import klib.unit_test;
import klib.core;

namespace {
TEST_CASE(assert) {
	[[maybe_unused]] static constexpr int value_v{42};
	auto thrown = false;
	try {
		KLIB_ASSERT_DEBUG(value_v == -5);
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
