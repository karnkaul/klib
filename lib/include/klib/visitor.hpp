#pragma once
#include <variant>

namespace klib {
template <typename... Ts>
struct Visitor : Ts... {
	using Ts::operator()...;
};

template <typename... Ts>
struct SubVisitor : Ts... {
	using Ts::operator()...;

	template <typename T>
	constexpr void operator()(T&& /*unused*/) const {}
};

template <typename VariantT, typename... Ts>
constexpr auto match(VariantT&& var, Ts&&... funcs) {
	return std::visit(Visitor{std::forward<Ts>(funcs)...}, std::forward<VariantT>(var));
}
} // namespace klib
