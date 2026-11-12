#include "klib/build_version.hpp"
#include "klib/core/constants.hpp"
#include "klib/core/version.hpp"
#include <print>

auto main() -> int { std::println("klib version: {}\nklib::debug_v: {}", klib::build_version_v, klib::debug_v); }
