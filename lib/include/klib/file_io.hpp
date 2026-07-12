#pragma once
#include "klib/concepts.hpp"
#include "klib/string/c_string.hpp"
#include <cstddef>
#include <cstring>
#include <span>
#include <vector>

namespace klib {
auto read_file_bytes_to(std::vector<std::byte>& out, CString path) -> bool;
auto read_file_bytes_to(std::string& out, CString path) -> bool;

template <typename ContainerT>
	requires(MemcpyAble<typename ContainerT::value_type>)
auto copy_file_bytes_to(ContainerT& out, CString const path) -> bool {
	static constexpr auto t_size_v = sizeof(typename ContainerT::value_type);
	auto bytes = std::vector<std::byte>{};
	if (!read_file_bytes_to(bytes, path) || bytes.size() % t_size_v != 0) { return false; }
	out.resize(bytes.size() / t_size_v);
	std::memcpy(out.data(), bytes.data(), bytes.size());
	return true;
}

auto write_bytes_to_file(std::span<std::byte const> bytes, CString path) -> bool;

template <MemcpyAble Type>
auto write_to_file(std::span<Type const> data, CString const path) -> bool {
	return write_bytes_to_file(std::as_bytes(data), path);
}

inline auto write_to_file(std::string_view const text, CString const path) -> bool { return write_to_file(std::span{text}, path); }

[[nodiscard]] auto resolve_symlink(std::string_view path, int max_iters = 100) -> std::string;
} // namespace klib
