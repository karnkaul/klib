#include <klib/byte_count.hpp>

namespace {
using namespace klib;
using namespace klib::literals;

static_assert(1_KiB == 1024_B);
static_assert(1_MiB == 1024_KiB);
static_assert(1_GiB == 1024_MiB);
static_assert(1_TiB == 1024_GiB);

static_assert(1_KB == 1000_B);
static_assert(1_MB == 1000_KB);
static_assert(1_GB == 1000_MB);
static_assert(1_TB == 1000_GB);

static_assert(1_KiB + 2_KiB == 3_KiB);
static_assert(3_KiB - 2_KiB == 1_KiB);
static_assert(3_KiB * 2_KiB == 6_KiB);
static_assert(4_KiB / 2_KiB == 2);
static_assert(4_KiB / 2 == 2_KiB);
} // namespace
