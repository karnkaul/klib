module;

#include "klib/debug.hpp"
#include <cstdlib>

export module klib.cli;

export import std;

export import klib.string;

import klib.core;

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

[[nodiscard]] auto line(std::string_view const message, std::move_only_function<bool(std::string)> pred) -> prompt::Selection {
	static constexpr auto max_attempts_v{3};
	auto line = std::string{};
	for (auto attempt = 0; attempt < max_attempts_v; ++attempt) {
		std::print("\n{}\n> ", message);
		std::getline(std::cin, line);
		std::println();
		if (pred(std::move(line))) { return Selection::Line; }
		std::println(std::cerr, "invalid input");
	}
	return Selection::Invalid;
}

[[nodiscard]] auto confirm(std::string_view const message) -> prompt::Selection {
	auto const msg = std::format("{} (y/N):", message);
	auto input = std::string{};
	auto const pred = [&](std::string in) {
		if (in.empty() || in == "n" || in == "y") {
			input = std::move(in);
			return true;
		}
		return false;
	};
	auto const selection = line(msg, pred);
	if (selection == Selection::Invalid) { return selection; }

	KLIB_ASSERT(selection == Selection::Line);
	if (input == "y") { return Selection::Confirm; }
	return Selection::Exit;
}

[[nodiscard]] auto options(std::span<prompt::Option const> options, bool const empty_is_exit) -> prompt::Selection {
	auto message = std::string{};
	auto number = 0;
	for (auto const& option : options) { std::format_to(std::back_inserter(message), "{}) {}\n", ++number, option.text); }
	message += "q) quit";

	auto const pred = [&](std::string_view input) {
		if (input.empty()) {
			if (!empty_is_exit) { return false; }
			input = "q";
		}
		if (input == "q") {
			number = 0;
			return true;
		}
		if (!FromChars{.text = input}(number)) { return false; }

		return number >= 1 && number <= int(options.size());
	};

	auto const selection = line(message, pred);
	if (selection == Selection::Invalid) { return selection; }

	KLIB_ASSERT(selection == Selection::Line);
	if (number == 0) { return Selection::Exit; }
	KLIB_ASSERT(number > 0);
	auto const index = std::size_t(number - 1);
	auto const& option = options[index];
	if (option.callback) { option.callback(); }
	return Selection::Option;
}
} // namespace klib::prompt

export namespace klib::cli {
class TextTable {
  public:
	enum class Align : std::int8_t { Left, Center, Right };

	class Builder;

	void push_row(std::vector<std::string> row) {
		m_rows.push_back(std::move(row));
		update_column_widths();
	}

	void push_separator() { push_row(separator()); }

	void append_to(std::string& out) const {
		for (auto const& column : m_columns) { column.fmt = make_column_fmt(column); }

		static constexpr std::string_view per_column_spacing_v{"|  "};
		static constexpr std::string_view end_spacing_v{"|"};
		auto const spacing = (m_columns.size() * per_column_spacing_v.size()) + end_spacing_v.size();
		auto const total_width =
			std::accumulate(m_columns.begin(), m_columns.end(), spacing, [](std::size_t const s, Column const& c) { return s + c.max_width; });
		append_border(out, total_width);
		append_titles(out);
		append_separator(out, total_width);
		for (auto const& row : m_rows) {
			if (row.empty()) {
				append_separator(out, total_width);
				continue;
			}

			for (std::size_t i = 0; i < m_columns.size(); ++i) {
				auto const& column = m_columns.at(i);
				auto const cell = i < row.size() ? row.at(i) : std::string_view{};
				append_cell(out, column.fmt, cell);
			}

			if (!no_border) { out += '|'; }
			out += '\n';
		}
		append_border(out, total_width);
	}

	[[nodiscard]] auto serialize() const -> std::string {
		auto ret = std::string{};
		append_to(ret);
		return ret;
	}

	bool no_border{};

  private:
	using Row = std::vector<std::string>;

	struct Column {
		std::string title{};
		Align align{Align::Left};
		std::size_t max_width{};
		mutable std::string fmt{};
	};

	static auto separator() -> Row { return {}; }

	[[nodiscard]] auto make_column_fmt(Column const& column) const -> std::string {
		auto ret = std::string{};
		auto const align_char = [align = column.align] {
			switch (align) {
			default:
			case TextTable::Align::Left: return '<';
			case TextTable::Align::Right: return '>';
			case TextTable::Align::Center: return '^';
			}
		}();
		std::string_view const prefix = no_border ? "" : "| ";
		std::format_to(std::back_inserter(ret), "{}{{:{}{}}} ", prefix, align_char, column.max_width);
		return ret;
	}

	void update_column_widths() {
		KLIB_ASSERT(!m_rows.empty());
		auto const& row = m_rows.back();
		if (row.empty()) { return; }
		for (auto [column, cell] : std::ranges::zip_view(m_columns, row)) { column.max_width = std::max(column.max_width, cell.size()); }
	}

	void append_border(std::string& out, std::size_t width) const {
		if (no_border || width < 2) { return; }
		out += '+';
		for (std::size_t i = 0; i < width - 2; ++i) { out += '-'; }
		out += "+\n";
	}

	void append_separator(std::string& out, std::size_t width) const {
		if (no_border) { return; }
		for (std::size_t i = 0; i < width; ++i) { out += '-'; }
		out += '\n';
	}

	static void append_cell(std::string& out, std::string_view fmt, std::string_view cell) {
		std::vformat_to(std::back_inserter(out), fmt, std::make_format_args(cell));
	}

	void append_titles(std::string& out) const {
		for (auto const& column : m_columns) { append_cell(out, column.fmt, column.title); }
		if (!no_border) { out += '|'; }
		out += '\n';
	}

	std::vector<Column> m_columns{};
	std::vector<Row> m_rows{};
};

class TextTable::Builder {
  public:
	auto add_column(std::string title, Align align = Align::Left) -> Builder& {
		auto column = Column{.title = std::move(title), .align = align};
		column.max_width = column.title.size();
		m_columns.push_back(std::move(column));
		return *this;
	}

	[[nodiscard]] auto build() const -> TextTable {
		auto ret = TextTable{};
		ret.m_columns = m_columns;
		return ret;
	}

  private:
	std::vector<Column> m_columns{};
};
} // namespace klib::cli

export namespace klib::shell {
enum struct ExitCode : int {};
constexpr auto success_v = ExitCode{EXIT_SUCCESS};
constexpr auto failure_v = ExitCode{EXIT_FAILURE};

constexpr std::string_view redirect_null_v =
#if defined(_WIN32)
	"NUL";
#else
	"/dev/null";
#endif

auto execute(CString const expression) -> ExitCode {
	if (expression.as_view().empty()) { return success_v; }
	// NOLINTNEXTLINE(concurrency-mt-unsafe)
	return ExitCode(std::system(expression.c_str()));
}

auto execute(std::string_view const expression, std::string_view const redirect) -> ExitCode {
	if (expression.empty()) { return success_v; }
	auto const redirected_expression = std::format("{} > {} 2>&1", expression, redirect);
	return execute(redirected_expression);
}

auto execute_silent(std::string_view const expression) -> ExitCode { return execute(expression, redirect_null_v); }
} // namespace klib::shell
