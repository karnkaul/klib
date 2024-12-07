#include <klib/debug_trap.hpp>
#include <klib/unit_test.hpp>

#include <print>

namespace {
TEST(debug_tracer_pid) {
	std::println("debugger attached: {}", klib::is_debugger_attached());
	KLIB_DEBUG_TRAP();
}
} // namespace
