#pragma once
#include <string>

namespace klib::env {
[[nodiscard]] auto exe_path() -> std::string const&;
} // namespace klib::env
