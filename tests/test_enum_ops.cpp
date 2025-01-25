#include <klib/enum_ops.hpp>
#include <cstdint>

namespace {
enum class Flag : std::int8_t {
	None = 0,
	One = 1 << 0,
	Two = 1 << 1,
};
} // namespace

namespace klib {
template <>
constexpr auto enable_enum_ops_v<Flag> = true;
} // namespace klib

namespace {
static_assert(Flag{} == Flag::None);
static_assert(((Flag::One | Flag::Two) & Flag::One) == Flag::One);
static_assert(((Flag::One | Flag::Two) & Flag::Two) == Flag::Two);
static_assert((((Flag::One | Flag::Two) & ~Flag::One) & Flag::One) == Flag::None);
static_assert((Flag::Two & Flag::One) == Flag::None);
static_assert((~Flag{} & Flag::One) == Flag::One);
static_assert((~Flag{} & Flag::Two) == Flag::Two);
static_assert((~Flag::Two & Flag::One) == Flag::One);
} // namespace
