export module klib.cli;

export import klib.core;

#pragma region prompt

export namespace klib::prompt {
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

[[nodiscard]] auto line(std::string_view message, std::move_only_function<bool(std::string)> pred) -> prompt::Selection;
[[nodiscard]] auto confirm(std::string_view message) -> prompt::Selection;
[[nodiscard]] auto options(std::span<prompt::Option const> options, bool empty_is_exit) -> prompt::Selection;
} // namespace klib::prompt

#pragma endregion

#pragma region shell

export namespace klib::shell {
enum struct ExitCode : int {};
inline constexpr auto success_v = ExitCode{0};
inline constexpr auto failure_v = ExitCode{1};

inline constexpr std::string_view redirect_null_v =
#ifdef _WIN32
	"NUL";
#else
	"/dev/null";
#endif

auto execute(CString expression) -> ExitCode;
auto execute(std::string_view expression, std::string_view redirect) -> ExitCode;
inline auto execute_silent(std::string_view const expression) -> ExitCode {
	return execute(expression, redirect_null_v);
}

auto open_directory(CString path) -> ExitCode;
} // namespace klib::shell

#pragma endregion
