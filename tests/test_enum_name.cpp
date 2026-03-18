#include "klib/enum/name.hpp"
#include "klib/unit_test/unit_test.hpp"
#include <cstdint>

namespace {
enum class Foo : std::int8_t { Alpha, Beta, Gamma };

TEST(enum_name) {
	auto const foo_name_map = klib::EnumNameMap<Foo>{
		{Foo::Alpha, "alpha"},
		{Foo::Beta, "beta"},
		{Foo::Gamma, "gamma"},
	};

	EXPECT(foo_name_map.to_name(Foo::Alpha) == "alpha");
	EXPECT(foo_name_map.to_name(Foo::Beta) == "beta");
	EXPECT(foo_name_map.to_name(Foo::Gamma) == "gamma");
	EXPECT(foo_name_map.to_name(Foo{42}).empty());

	EXPECT(foo_name_map.to_enum("alpha") == Foo::Alpha);
	EXPECT(foo_name_map.to_enum("beta") == Foo::Beta);
	EXPECT(foo_name_map.to_enum("gamma") == Foo::Gamma);
	EXPECT(!foo_name_map.to_enum("foo").has_value());
}
} // namespace
