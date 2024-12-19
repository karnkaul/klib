#include <klib/fixed_any.hpp>
#include <klib/meta.hpp>
#include <klib/unit_test.hpp>
#include <string>
#include <vector>

namespace {
using namespace klib;

TEST(fixed_any_literals) {
	auto x = FixedAny{};
	x = 42;
	ASSERT(x.contains<int>());
	EXPECT(x.get<int>() == 42);

	x = 'c';
	ASSERT(x.contains<char>());
	EXPECT(x.get<char>() == 'c');
}

TEST(fixed_any_classes) {
	static constexpr auto size_v = sizeof(LargestOf<std::string, std::vector<int>>);
	auto x = FixedAny<size_v>{};
	x = std::string{"foo"};
	ASSERT(x.contains<std::string>());
	EXPECT(x.get<std::string>() == "foo");

	x = std::vector<int>{0, 1, 2};
	ASSERT(x.contains<std::vector<int>>());
	auto const& v = x.get<std::vector<int>>();
	ASSERT(v.size() == 3);
	EXPECT(v[0] == 0 && v[1] == 1 && v[2] == 2);
}
} // namespace
