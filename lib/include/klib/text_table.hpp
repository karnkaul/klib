#pragma once
#include <string>
#include <vector>

namespace klib {
class TextTable {
  public:
	enum class Align : int { Left, Center, Right };

	class Builder;

	void push_row(std::vector<std::string> row);
	void push_separator() { push_row(separator()); }

	void append_to(std::string& out) const;

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

	[[nodiscard]] auto make_column_fmt(Column const& column) const -> std::string;

	void update_column_widths();

	void append_border(std::string& out, std::size_t width) const;
	void append_separator(std::string& out, std::size_t width) const;

	static void append_cell(std::string& out, std::string_view fmt, std::string_view cell);
	void append_titles(std::string& out) const;

	std::vector<Column> m_columns{};
	std::vector<Row> m_rows{};
};

class TextTable::Builder {
  public:
	auto add_column(std::string title, Align align = Align::Left) -> Builder&;

	[[nodiscard]] auto build() const -> TextTable;

  private:
	std::vector<Column> m_columns{};
};
} // namespace klib
