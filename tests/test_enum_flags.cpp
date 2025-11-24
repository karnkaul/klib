#include "klib/enum_flags.hpp"
#include <cstdint>

namespace {
enum class Flag : std::int8_t {
	None = 0,
	One = 1 << 0,
	Two = 1 << 1,
};
using Flags = klib::EnumFlags<Flag>;

static_assert(Flags{} == Flag::None);
static_assert((Flags{Flag::One, Flag::Two} & Flag::One) == Flag::One);
static_assert((Flags{Flag::One, Flag::Two} & Flag::Two) == Flag::Two);
static_assert(((Flags{Flag::One, Flag::Two} & ~Flags{Flag::One}) & Flag::One) == Flag::None);
static_assert((Flags{Flag::Two} & Flag::One) == Flag::None);
static_assert((~Flags{} & Flag::One) == Flag::One);
static_assert((~Flags{} & Flag::Two) == Flag::Two);
static_assert((~Flags{Flag::Two} & Flag::One) == Flag::One);
} // namespace
