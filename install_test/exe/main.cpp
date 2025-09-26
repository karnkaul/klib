#include <klib/build_version.hpp>
#include <klib/version_str.hpp>
#include <print>

auto main() -> int { std::println("klib version: {}", klib::build_version_v); }
