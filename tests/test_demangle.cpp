#include <klib/c_string.hpp>
#include <klib/demangle.hpp>
#include <klib/unit_test.hpp>

namespace {
TEST(demangle) { EXPECT(klib::demangled_name<klib::CString>() == "klib::CString"); }
} // namespace
