#include <klib/assert.hpp>
#include <klib/unit_test.hpp>
#include <print>
#include <thread>

namespace {
TEST(assert) {
	constexpr std::string_view x{"hello"};
	KLIB_ASSERT(x == "world");
}
} // namespace
