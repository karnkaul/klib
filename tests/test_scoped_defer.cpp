#include "klib/scoped_defer.hpp"
#include "klib/unit_test.hpp"

namespace {
using namespace klib;
TEST(scoped_defer_exec) {
	auto value = 0;
	{
		value = 1;
		auto to42 = ScopedDefer{[&] { value = 42; }};
		value = 2;
	}
	EXPECT(value == 42);
}

TEST(scoped_defer_move) {
	auto value = 0;
	{
		auto to42 = ScopedDefer{};
		value = 1;
		to42 = [&] { value = 42; };
		auto moved = std::move(to42);
		value = 2;
	}
	EXPECT(value == 42);
}
} // namespace
