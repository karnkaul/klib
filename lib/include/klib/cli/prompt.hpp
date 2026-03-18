#pragma once
#include <cstdint>
#include <functional>
#include <span>
#include <string>
#include <string_view>

namespace vifo {
namespace prompt {
enum class Selection : std::int8_t {
	Exit = 0,
	Invalid = 1,
	Option = 2,
	Confirm = Option,
	Line = Option,
};

struct Option {
	std::string_view text{};
	std::function<void()> callback{};
};
} // namespace prompt

namespace util {
[[nodiscard]] auto prompt_line(std::string_view message, std::move_only_function<bool(std::string)> pred) -> prompt::Selection;
[[nodiscard]] auto prompt_confirm(std::string_view message) -> prompt::Selection;
[[nodiscard]] auto prompt_options(std::span<prompt::Option const> options, bool empty_is_exit) -> prompt::Selection;
} // namespace util
} // namespace vifo
