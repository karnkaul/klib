#include "klib/build_version.hpp"
#include "klib/constants.hpp"
#include "klib/version_str.hpp"
#include <print>

auto main() -> int { std::println("klib version: {}\nklib::debug_v: {}", klib::build_version_v, klib::debug_v); }
