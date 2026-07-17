#pragma once
#include "klib/concepts.hpp"
#include <charconv>
#include <string_view>

namespace klib {
namespace detail {
template <NumberT Type, typename... Args>
auto do_from_chars(std::string_view& out_text, Type& out_number, Args const... args) -> bool {
	auto const* end = out_text.data() + out_text.size();
	auto const [ptr, ec] = std::from_chars(out_text.data(), end, out_number, args...);
	if (ec != std::errc{}) { return false; }
	out_text = (ptr == end) ? std::string_view{} : std::string_view{ptr, std::size_t(end - ptr)};
	return true;
}
} // namespace detail

struct FromChars {
	template <std::integral Type>
	auto operator()(Type& out, int const base = 10) -> bool {
		return detail::do_from_chars(text, out, base);
	}

	template <std::floating_point Type>
	auto operator()(Type& out) -> bool {
		return detail::do_from_chars(text, out);
	}

	[[nodiscard]] auto advance_if(char ch) -> bool;
	[[nodiscard]] auto advance_if_any(std::string_view chars) -> bool;
	[[nodiscard]] auto advance_if_all(std::string_view str) -> bool;

	std::string_view text{};
};

template <std::integral Type>
auto try_parse_to(Type& out, std::string_view const text, int const base = 10) -> bool {
	return FromChars{.text = text}(out, base);
}

template <std::floating_point Type>
auto try_parse_to(Type& out, std::string_view const text) -> bool {
	return FromChars{.text = text}(out);
}

template <NumberT Type>
[[nodiscard]] auto string_to_number(std::string_view const text, Type const& fallback = {}) -> Type {
	auto ret = Type{};
	if (!FromChars{.text = text}(ret)) { return fallback; }
	return ret;
}
} // namespace klib
