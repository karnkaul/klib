#pragma once
#include <klib/concepts.hpp>
#include <string>
#include <typeinfo>

namespace klib {
[[nodiscard]] auto demangled_name(std::type_info const& info) -> std::string;

template <typename Type>
[[nodiscard]] auto demangled_name() -> std::string const& {
	static std::string const ret = [] { return demangled_name(typeid(Type)); }();
	return ret;
}

template <PolymorphicT Type>
[[nodiscard]] auto demangled_name(Type const& t) -> std::string {
	return demangled_name(typeid(t));
}
} // namespace klib
