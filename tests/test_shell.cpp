#include "klib/unit_test/unit_test.hpp"

import klib.cli;
import klib.unit_test;

namespace {
TEST_CASE(shell_execute) {
	static constexpr klib::CString expr_v{"git status"};
	auto result = klib::shell::execute(expr_v);
	EXPECT(result == klib::shell::success_v);

	result = klib::shell::execute_silent(expr_v.as_view());
	EXPECT(result == klib::shell::success_v);
}
} // namespace
