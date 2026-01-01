#pragma once
#include "klib/c_string.hpp"
#include <fstream>

namespace klib {
template <typename ContainerT>
auto read_file_bytes_into(ContainerT& out, CString const path) -> bool {
	using value_type = ContainerT::value_type;
	auto file = std::ifstream{path.c_str(), std::ios::binary | std::ios::ate};
	if (!file.is_open()) { return false; }
	auto const size = file.tellg();
	if (std::size_t(size) % sizeof(value_type) != 0) { return false; }
	file.seekg(0, std::ios::beg);
	out.resize(std::size_t(size) / sizeof(value_type));
	void* first = out.data();
	file.read(static_cast<char*>(first), size);
	return true;
}

[[nodiscard]] auto resolve_symlink(std::string_view path, int max_iters = 100) -> std::string;
} // namespace klib
