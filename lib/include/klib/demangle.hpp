#pragma once
#include <string>
#include <typeinfo>

namespace klib {
[[nodiscard]] auto demangled_name(std::type_info const& info) -> std::string;

template <typename Type>
[[nodiscard]] auto demangled_name() -> std::string_view {
	static std::string const ret = [] { return demangled_name(typeid(Type)); }();
	return ret;
}
} // namespace klib
