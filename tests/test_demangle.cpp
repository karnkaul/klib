#include "klib/unit_test/unit_test.hpp"

import klib.unit_test;
import klib.core;
import klib.string;

namespace test {
// NOLINTNEXTLINE(cppcoreguidelines-special-member-functions)
struct Base {
	virtual ~Base() = default;
};

struct Derived : Base {};
} // namespace test

namespace {
TEST_CASE(demangle) {
	auto name = klib::demangled_name<klib::CString>();
	EXPECT(name == "klib::CString");

	auto const derived = test::Derived{};
	test::Base const& base = derived;
	name = klib::demangled_name(base);
	EXPECT(name == "test::Derived");
}
} // namespace
