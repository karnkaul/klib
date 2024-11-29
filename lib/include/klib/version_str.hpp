#pragma once
#include <klib/c_string.hpp>
#include <klib/version.hpp>
#include <string>

namespace klib {
void append_to(std::string& out, Version const& version);

[[nodiscard]] auto to_string(Version const& version) -> std::string;

[[nodiscard]] auto to_version(CString text) -> Version;
} // namespace klib
