#include "klib/enum_array.hpp"

namespace {
using namespace klib;

enum class Foo : int { Zero, One, Two, COUNT_ };

constexpr auto foo_v = EnumArray<Foo, int>{0, 1, 2};
static_assert(foo_v[Foo::Zero] == 0 && foo_v[Foo::One] == 1 && foo_v[Foo::Two] == 2);
static_assert(foo_v.at(Foo::Zero) == 0);
} // namespace
