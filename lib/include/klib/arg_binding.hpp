#pragma once
#include <klib/concepts.hpp>
#include <charconv>
#include <vector>

namespace klib {
using ArgAssignment = bool (*)(void* binding, std::string_view value);
using ArgAsString = std::string (*)(void const* binding);

inline auto assign_to_arg(bool& out, std::string_view /*value*/) -> bool {
	out = true;
	return true;
}

template <NumberT Type>
auto assign_to_arg(Type& out, std::string_view const value) {
	auto const* last = value.data() + value.size();
	auto const [ptr, ec] = std::from_chars(value.data(), last, out);
	return ptr == last && ec == std::errc{};
}

template <StringyT Type>
auto assign_to_arg(Type& out, std::string_view const value) -> bool {
	out = value;
	return true;
}

template <typename Type>
auto assign_to(std::vector<Type>& out, std::string_view const value) -> bool {
	auto t = Type{};
	if (!assign_to_arg(t, value)) { return false; }
	out.push_back(std::move(t));
	return true;
}

template <typename Type>
auto arg_as_string(Type const& t) -> std::string {
	if constexpr (std::constructible_from<std::string, Type>) {
		return std::string{t};
	} else {
		using std::to_string;
		return to_string(t);
	}
}

template <typename Type>
auto arg_as_string(std::vector<Type> const& /*vec*/) -> std::string {
	return "...";
}

struct ArgBinding {
	ArgAssignment assign{};
	ArgAsString to_string{};

	template <typename Type>
	static auto create() -> ArgBinding {
		return ArgBinding{
			.assign = [](void* binding, std::string_view const value) -> bool { return assign_to_arg(*static_cast<Type*>(binding), value); },
			.to_string = [](void const* binding) -> std::string { return arg_as_string(*static_cast<Type const*>(binding)); },
		};
	}
};
} // namespace klib
