#include "klib/unique.hpp"

namespace {
using namespace klib;

consteval auto unique_int() {
	auto i = Unique<int>{};
	i = 42;
	auto j = std::move(i);
	if (!i.is_identity()) { return false; }
	if (i.get() != 0) { return false; }
	return j.get() == 42;
}
static_assert(unique_int());

consteval auto custom_id() {
	struct Id {
		constexpr auto operator()(int const i) const { return i == -1; }
	};
	auto i = Unique<int, Noop<int>, Id>{-1};
	if (!i.is_identity()) { return false; }
	i = 42;
	if (i.is_identity()) { return false; }
	return i.get() == 42;
}
static_assert(custom_id());

consteval auto custom_deleter() {
	auto deleted = int{};
	auto called = int{};

	{
		struct Deleter {
			int* deleted{};
			int* called{};
			constexpr void operator()(int const i) const {
				if (deleted == nullptr || called == nullptr) { return; }
				*deleted = i;
				++(*called);
			}
		};
		auto i = Unique<int, Deleter>{1, {.deleted = &deleted, .called = &called}};
		if (deleted != 0 || called != 0) { return false; }
		i = {42, {.deleted = &deleted, .called = &called}};
		if (deleted != 1 || called != 1) { return false; }
	}

	return called == 2 && deleted == 42;
}
static_assert(custom_deleter());
} // namespace
