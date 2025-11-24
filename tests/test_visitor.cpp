#include "klib/visitor.hpp"
#include <variant>

namespace {
struct A {};
struct B {
	int value{};
};

static_assert([] {
	auto v = std::variant<A, B>{B{.value = 42}};
	auto const visitor = klib::Visitor{
		[](B const b) { return b.value == 42; },
		[](A const&) { return false; },
	};
	return std::visit(visitor, v);
}());

static_assert([] {
	auto v = std::variant<A, B>{A{}};
	auto ret = false;
	auto const visitor = klib::SubVisitor{
		[&ret](A) { ret = true; },
	};
	std::visit(visitor, v);
	return ret;
}());

static_assert([]() {
	auto const v = std::variant<A, B>{A{}};
	return klib::match(v, [](A) { return true; }, [](B) { return false; });
}());
} // namespace
