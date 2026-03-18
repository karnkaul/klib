#pragma once
#include "klib/string/c_string.hpp"
#include <string>

namespace klib::env {
[[nodiscard]] auto exe_path() -> std::string const&;

[[nodiscard]] auto get_var(CString key) -> CString;
} // namespace klib::env
