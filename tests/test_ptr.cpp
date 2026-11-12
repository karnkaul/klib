import klib.core;

#include "klib/unit_test/unit_test.hpp"

namespace {
using klib::Ptr;

struct A {};
struct B : A {};

constexpr auto a = A{};
constexpr auto p_a = Ptr{&a};
static_assert(p_a);
static_assert(p_a == &a);

constexpr auto b = B{};
constexpr Ptr<A const> p_b{&b};
static_assert(p_b);
static_assert(p_b == &b);
static_assert(p_a != klib::Ptr{&b});

constexpr Ptr<A const> null = nullptr;
static_assert(null == nullptr);

TEST_CASE(ptr_assert) {
	struct S {
		int value{};
	};

	auto ptr = klib::Ptr<S>{};
	auto thrown = false;
	klib::assertion::set_fail_action(klib::assertion::FailAction::Throw);
	try {
		[[maybe_unused]] auto const s = *ptr;
		[[maybe_unused]] auto const i = ptr->value;
	} catch (klib::assertion::Failure const&) {
		thrown = true;
	}
	EXPECT(thrown);
}
} // namespace
