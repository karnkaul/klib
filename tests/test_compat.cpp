#include <klib/compat.hpp>

namespace {
using namespace klib;

static_assert(abs(-1) == 1);
static_assert(abs(-3.14f) == 3.14f);
static_assert(abs(3.14f) == 3.14f);
} // namespace
