#include "klib/c_string.hpp"
#include "klib/demangle.hpp"
#include "klib/unit_test.hpp"

namespace test {
// NOLINTNEXTLINE(cppcoreguidelines-special-member-functions)
struct Base {
	virtual ~Base() = default;
};

struct Derived : Base {};
} // namespace test

namespace {
TEST(demangle) {
	auto name = klib::demangled_name<klib::CString>();
	EXPECT(name == "klib::CString");

	auto const derived = test::Derived{};
	test::Base const& base = derived;
	name = klib::demangled_name(base);
	EXPECT(name == "test::Derived");
}
} // namespace
