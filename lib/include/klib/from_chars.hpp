#pragma once
#include <klib/concepts.hpp>
#include <charconv>
#include <string_view>

namespace klib {
struct FromChars {
	template <NumberT Type>
	auto operator()(Type& out) -> bool {
		auto const* end = text.data() + text.size();
		auto const [ptr, ec] = std::from_chars(text.data(), end, out);
		if (ec != std::errc{}) { return false; }
		text = (ptr == end) ? std::string_view{} : std::string_view{ptr, std::size_t(end - ptr)};
		return true;
	}

	[[nodiscard]] auto advance_if(char ch) -> bool;
	[[nodiscard]] auto advance_if_any(std::string_view chars) -> bool;
	[[nodiscard]] auto advance_if_all(std::string_view str) -> bool;

	std::string_view text{};
};
} // namespace klib
