#pragma once
#include "klib/constants.hpp"
#include "klib/enum/array.hpp"
#include "klib/string/escape_code.hpp"
#include <cstdint>
#include <format>
#include <optional>
#include <source_location>
#include <string>
#include <string_view>

namespace klib {
namespace log {
enum class Level : std::int8_t { Error, Warn, Info, Debug, COUNT_ };
inline constexpr auto level_to_char = EnumArray<Level, char>{'E', 'W', 'I', 'D'};
inline constexpr auto debug_enabled_v = debug_v;

using Colors = EnumArray<Level, std::optional<escape::Rgb>>;
inline constexpr auto colors_v = Colors{
	escape::Rgb{241, 76, 76},
	escape::Rgb{245, 245, 67},
	escape::Rgb{229, 229, 229},
	escape::Rgb{170, 170, 170},
};

template <typename... Args>
struct BasicFmt : std::basic_format_string<char, Args...> {
	template <std::convertible_to<std::string_view> T>
	consteval BasicFmt(T const& t, std::source_location const& sloc = std::source_location::current())
		: std::basic_format_string<char, Args...>(t), sloc(sloc) {}

	std::source_location sloc;
};

template <typename... Args>
using Fmt = BasicFmt<std::type_identity_t<Args>...>;

template <typename... Args>
void print(Level level, std::string_view tag, Fmt<Args...> const& fmt, Args&&... args);

template <typename... Args>
void error(std::string_view tag, Fmt<Args...> const& fmt, Args&&... args) {
	print(Level::Error, tag, fmt, std::forward<Args>(args)...);
}

template <typename... Args>
void warn(std::string_view tag, Fmt<Args...> const& fmt, Args&&... args) {
	print(Level::Warn, tag, fmt, std::forward<Args>(args)...);
}

template <typename... Args>
void info(std::string_view tag, Fmt<Args...> const& fmt, Args&&... args) {
	print(Level::Info, tag, fmt, std::forward<Args>(args)...);
}

template <typename... Args>
void debug(std::string_view tag, Fmt<Args...> const& fmt, Args&&... args) {
	if constexpr (!debug_enabled_v) { return; }
	print(Level::Debug, tag, fmt, std::forward<Args>(args)...);
}

// NOLINTNEXTLINE(performance-enum-size)
enum struct ThreadId : std::int64_t { Main = 0 };

struct Input {
	Level level{};
	std::string_view tag{};
	std::string_view message{};
	std::string_view file_name{};
	std::uint64_t line_number{};
};

constexpr auto debug_interpolate_format_v = std::string_view{"[{level}] [{tag}/{thread_id}] {message} [{timestamp}] [{file_name}:{line_number}]"};
constexpr auto ndebug_interpolate_format_v = std::string_view{"[{level}] [{tag}/{thread_id}] {message} [{timestamp}]"};
constexpr auto interpolate_format_v = debug_v ? debug_interpolate_format_v : ndebug_interpolate_format_v;

void set_max_level(Level level);
[[nodiscard]] auto get_max_level() -> Level;

void set_colors(std::optional<Colors> const& colors);
[[nodiscard]] auto get_colors() -> std::optional<Colors>;

void set_interpolate_format(std::string_view interpolate_format);

[[nodiscard]] auto get_thread_id() -> ThreadId;

[[nodiscard]] auto format(Input const& input) -> std::string;
void print(Input const& input);
} // namespace log

template <typename... Args>
void log::print(Level const level, std::string_view const tag, Fmt<Args...> const& fmt, Args&&... args) {
	if (level > get_max_level()) { return; }
	auto const message = std::format(fmt, std::forward<Args>(args)...);
	auto const input = Input{
		.level = level,
		.tag = tag,
		.message = message,
		.file_name = fmt.sloc.file_name(),
		.line_number = fmt.sloc.line(),
	};
	print(input);
}
} // namespace klib
