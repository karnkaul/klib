#include <klib/c_string.hpp>
#include <klib/demangle.hpp>
#include <klib/unit_test.hpp>

namespace {
TEST(demangle) {
	auto const& name = klib::demangled_name<klib::CString>();
	EXPECT(name == "klib::CString");
}
} // namespace
