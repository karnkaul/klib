#pragma once
#include <args/scanner.hpp>
#include <klib/args/parse_info.hpp>
#include <klib/args/parse_result.hpp>

namespace klib::args {
class Parser {
  public:
	using Result = ParseResult;

	explicit Parser(ParseInfo const& info, std::string_view exe_name, std::span<char const* const> cli_args);

	explicit Parser(std::span<char const* const> cli_args, IPrinter* printer) : Parser(ParseInfo{.printer = printer}, {}, cli_args) {}

	[[nodiscard]] auto parse(std::span<Arg const> args) -> Result;

  private:
	struct Cursor {
		ParamCommand const* cmd{};
		std::size_t next_pos{};
	};

	struct Printer : IPrinter {
		void print(std::string_view text) final;
		void printerr(std::string_view text) final;
	};

	auto select_command() -> Result;
	auto parse_next() -> Result;
	auto parse_option() -> Result;
	auto parse_letters() -> Result;
	auto parse_word() -> Result;
	auto parse_last_option(ParamOption const& option, std::string_view input) -> Result;
	auto parse_argument() -> Result;
	auto parse_positional() -> Result;

	[[nodiscard]] auto try_builtin(std::string_view word) const -> bool;
	[[nodiscard]] auto find_option(char letter) const -> ParamOption const*;
	[[nodiscard]] auto find_option(std::string_view word) const -> ParamOption const*;
	[[nodiscard]] auto find_command(std::string_view name) const -> ParamCommand const*;

	[[nodiscard]] auto next_positional() -> ParamPositional const*;

	[[nodiscard]] auto check_required() -> Result;

	[[nodiscard]] auto cmd_name() const -> std::string_view { return m_cursor.cmd == nullptr ? "" : m_cursor.cmd->name; }
	[[nodiscard]] auto help_string() const -> std::string;
	[[nodiscard]] auto usage_string() const -> std::string;

	ParseInfo m_info{};
	std::string_view m_exe_name{};

	Scanner m_scanner;
	std::span<Arg const> m_args{};
	Cursor m_cursor{};
	bool m_has_commands{};
};
} // namespace klib::args
