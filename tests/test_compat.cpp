#include "klib/compat.hpp"

namespace {
static_assert(klib::abs(-1) == 1);
static_assert(klib::abs(-3.14f) == 3.14f);
static_assert(klib::abs(3.14f) == 3.14f);
} // namespace
