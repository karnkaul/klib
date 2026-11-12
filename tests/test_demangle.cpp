import klib.core.string;

#include "klib/unit_test/unit_test.hpp"

namespace test {
// NOLINTNEXTLINE(cppcoreguidelines-special-member-functions,misc-use-internal-linkage)
struct Base {
	virtual ~Base() = default;
};

// NOLINTNEXTLINE(misc-use-internal-linkage)
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
