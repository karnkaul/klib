#pragma once
#include <klib/arg.hpp>
#include <cstdlib>
#include <optional>

namespace klib {
/// \brief Error parsing passed arguments.
enum class ArgParseError : int {
	InvalidCommand = 100,
	InvalidOption,
	InvalidArgument,
	MissingArgument,
};

struct ArgParseExecutedBuiltin {};

/// \brief Result of parsing args / running selected command.
class ArgParseResult {
  public:
	ArgParseResult() = default;

	constexpr ArgParseResult(std::string_view command_name) : m_command_name(command_name) {}
	constexpr ArgParseResult(ArgParseError parse_error) : m_parse_error(parse_error) {}
	constexpr ArgParseResult(ArgParseExecutedBuiltin const& /*eb*/) : m_executed_builtin(true) {}

	/// \brief Check if clap executed a builtin option (like "--help")
	/// \returns true if clap executed a builtin option.
	[[nodiscard]] constexpr auto executed_builtin() const -> bool { return m_executed_builtin; }

	/// \brief Get the entered command name.
	/// \returns Entered command name matching passed args, if any.
	[[nodiscard]] constexpr auto get_command_name() const -> std::string_view { return m_command_name; }

	/// \brief Get the error that occurred during argument parsing.
	/// \returns Argument parsing error, if any.
	[[nodiscard]] constexpr auto get_parse_error() const -> std::optional<ArgParseError> { return m_parse_error; }

	/// \brief Get the return code for main.
	/// \returns EXIT_FAILURE on parse error, else EXIT_SUCCESS.
	[[nodiscard]] constexpr auto get_return_code() const -> int { return m_parse_error ? int(*m_parse_error) : EXIT_SUCCESS; }

	/// \brief Check if result should be returned early.
	/// \returns true if a builtin was executed or a parse error occurred.
	[[nodiscard]] constexpr auto early_return() const -> bool { return executed_builtin() || m_parse_error.has_value(); }

	/// \brief Conversion to bool.
	/// \returns true if early_return() returns false.
	constexpr explicit operator bool() const { return !early_return(); }

	/// \brief Compare equality with another ParseResult.
	auto operator==(ArgParseResult const& rhs) const -> bool = default;

  private:
	std::string_view m_command_name{};
	std::optional<ArgParseError> m_parse_error{};
	bool m_executed_builtin{};
};
} // namespace klib
