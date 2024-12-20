#include <klib/meta.hpp>
#include <concepts>
#include <cstdint>

namespace {
using namespace klib;

static_assert(std::same_as<LargestOf<int, short, double, char>, double>);
static_assert(std::same_as<LargestOf<std::int32_t, std::int64_t>, std::int64_t>);
static_assert(std::same_as<SmallestOf<int, short, double, char>, char>);
static_assert(std::same_as<SmallestOf<std::int32_t, std::int64_t>, std::int32_t>);
} // namespace
