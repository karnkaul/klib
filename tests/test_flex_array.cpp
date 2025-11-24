#include "klib/flex_array.hpp"
#include <algorithm>
#include <ranges>
#include <span>

namespace {
using namespace klib;

static_assert([] {
	auto arr = FlexArray<int, 3>{1, 2};
	if (arr.size() != 2) { return false; }
	if (arr.front() != 1) { return false; }
	if (arr.back() != 2) { return false; }
	auto reversed = arr;
	std::ranges::reverse(reversed);
	for (auto const [a, b] : std::ranges::zip_view{reversed, std::ranges::reverse_view{arr}}) {
		if (a != b) { return false; }
	}
	auto span = std::span{arr};
	if (span.size() != 2) { return false; }
	if (span[0] != arr[0] || span[1] != arr[1]) { return false; }
	return true;
}());

static_assert([] {
	auto arr = FlexArray<int, 2>{1};
	if (arr.size() != 1) { return false; }
	if (arr.front() != arr.back()) { return false; }
	if (!arr.try_push_back(2)) { return false; }
	if (arr.try_push_back(3)) { return false; }
	if (arr[0] != 1) { return false; }
	if (arr[1] != 2) { return false; }
	int x{};
	if (!arr.try_pop_back(x)) { return false; }
	if (x != 2) { return false; }
	if (!arr.try_pop_back(x)) { return false; }
	if (x != 1) { return false; }
	if (arr.try_pop_back(x)) { return false; }
	return true;
}());
} // namespace
