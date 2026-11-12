#include "klib/core/assert.hpp"
#include "klib/core/string/demangle.hpp"
#include "klib/core/string/escape_code.hpp"
#include "klib/core/string/from_chars.hpp"
#include "klib/core/string/glob.hpp"
#include "klib/core/string/lerp_expr/token.hpp"
#include "klib/core/string/text_table.hpp"
#include <algorithm>
#include <numeric>

#if __has_include(<cxxabi.h>)
#define KLIB_USE_CXA_DEMANGLE
#include <cxxabi.h>
#endif

#pragma region text_table

namespace klib {
void TextTable::push_row(std::vector<std::string> row) {
	m_rows.push_back(std::move(row));
	update_column_widths();
}

void TextTable::append_to(std::string& out) const {
	for (auto const& column : m_columns) {
		column.fmt = make_column_fmt(column);
	}

	static constexpr std::string_view per_column_spacing_v{"|  "};
	static constexpr std::string_view end_spacing_v{"|"};
	auto const spacing = (m_columns.size() * per_column_spacing_v.size()) + end_spacing_v.size();
	auto const total_width =
		std::accumulate(m_columns.begin(), m_columns.end(), spacing, [](std::size_t const s, Column const& c) {
			return s + c.max_width;
		});
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

		if (!no_border) {
			out += '|';
		}
		out += '\n';
	}
	append_border(out, total_width);
}

auto TextTable::serialize() const -> std::string {
	auto ret = std::string{};
	append_to(ret);
	return ret;
}

auto TextTable::make_column_fmt(Column const& column) const -> std::string {
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

void TextTable::update_column_widths() {
	klib_assert(!m_rows.empty());
	auto const& row = m_rows.back();
	if (row.empty()) {
		return;
	}
	for (auto [column, cell] : std::ranges::zip_view(m_columns, row)) {
		column.max_width = std::max(column.max_width, cell.size());
	}
}

void TextTable::append_border(std::string& out, std::size_t const width) const {
	if (no_border || width < 2) {
		return;
	}
	out += '+';
	for (std::size_t i = 0; i < width - 2; ++i) {
		out += '-';
	}
	out += "+\n";
}

void TextTable::append_separator(std::string& out, std::size_t const width) const {
	if (no_border) {
		return;
	}
	for (std::size_t i = 0; i < width; ++i) {
		out += '-';
	}
	out += '\n';
}

void TextTable::append_cell(std::string& out, std::string_view const fmt, std::string_view const cell) {
	std::vformat_to(std::back_inserter(out), fmt, std::make_format_args(cell));
}

void TextTable::append_titles(std::string& out) const {
	for (auto const& column : m_columns) {
		append_cell(out, column.fmt, column.title);
	}
	if (!no_border) {
		out += '|';
	}
	out += '\n';
}

auto TextTable::Builder::add_column(std::string title, Align const align) -> Builder& {
	auto column = Column{.title = std::move(title), .align = align};
	column.max_width = column.title.size();
	m_columns.push_back(std::move(column));
	return *this;
}

auto TextTable::Builder::build() const -> TextTable {
	auto ret = TextTable{};
	ret.m_columns = m_columns;
	return ret;
}
} // namespace klib

#pragma endregion

#pragma region escape_code

namespace klib {
namespace escape {
namespace {
[[nodiscard]] auto colorify(Rgb const rgb, int const target) -> FixedString<> {
	// NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)
	return {"{}{};2;{};{};{}{}", prefix_v, target, rgb[0], rgb[1], rgb[2], suffix_v};
}
} // namespace
} // namespace escape

auto escape::foreground(Rgb const rgb) -> FixedString<> { return colorify(rgb, 38); }
auto escape::background(Rgb const rgb) -> FixedString<> { return colorify(rgb, 48); }
} // namespace klib

#pragma endregion

#pragma region FromChars

namespace klib {
auto FromChars::advance_if(char const ch) -> bool {
	if (text.empty() || text.front() != ch) {
		return false;
	}
	text.remove_prefix(1);
	return true;
}

auto FromChars::advance_if_any(std::string_view const chars) -> bool {
	if (chars.empty() || text.empty()) {
		return false;
	}
	return std::ranges::any_of(chars, [this](char const ch) {
		return advance_if(ch);
	});
}

auto FromChars::advance_if_all(std::string_view const str) -> bool {
	if (text.empty() || str.empty() || !text.starts_with(str)) {
		return false;
	}
	text.remove_prefix(str.size());
	return true;
}
} // namespace klib

#pragma endregion

#pragma region demangle

namespace {
auto demangled_name_impl(char const* type_name) -> std::string {
#ifdef KLIB_USE_CXA_DEMANGLE
	auto status = int{};
	auto const buf = std::unique_ptr<char, decltype(std::free)*>{
		abi::__cxa_demangle(type_name, nullptr, nullptr, &status),
		std::free,
	};
	if (status == 0) {
		return buf.get();
	}
#endif
	static constexpr auto prefixes_v = std::array{
		"struct ",
		"class ",
		"enum ",
	};
	auto view = std::string_view{type_name};
	for (std::string_view const prefix : prefixes_v) {
		if (view.starts_with(prefix)) {
			view.remove_prefix(prefix.size());
			break;
		}
	}
	return std::string{view};
}
} // namespace

auto klib::demangled_name(std::type_info const& info) -> std::string { return demangled_name_impl(info.name()); }
auto klib::demangled_name(std::type_index const index) -> std::string { return demangled_name_impl(index.name()); }

#undef KLIB_USE_CXA_DEMANGLE

#pragma endregion

#pragma region glob

namespace klib {
namespace glob {
namespace {
constexpr auto needs_escape_v = std::array{'.', '+', '^', '$', '|', '(', ')', '{', '}'};
constexpr auto needs_dot_v = std::array{'*', '?'};

[[nodiscard]] constexpr auto contains(std::span<char const> chars, char const c) {
	return std::ranges::find(chars, c) != chars.end();
}
[[nodiscard]] constexpr auto needs_escape(char const c) { return contains(needs_escape_v, c); }
[[nodiscard]] constexpr auto needs_dot(char const c) { return contains(needs_dot_v, c); }

[[nodiscard]] constexpr auto is_open_brace(char const c) { return c == '['; }
[[nodiscard]] constexpr auto is_glob_not(char const in) { return in == '!'; }
constexpr auto regex_not_v{'^'};

[[nodiscard]] auto to_regex(std::string_view const input) -> std::string {
	auto ret = std::string{};

	auto prev_was_open_brace = false;
	for (char c : input) {
		if (prev_was_open_brace && is_glob_not(c)) {
			c = regex_not_v;
		} else if (needs_escape(c)) {
			ret.push_back('\\');
		} else if (needs_dot(c)) {
			ret.push_back('.');
		}

		prev_was_open_brace = is_open_brace(c);

		ret.push_back(c);
	}

	return ret;
}
} // namespace
} // namespace glob

auto Glob::create(std::string_view const expression) -> std::expected<Glob, Error> {
	if (expression.empty()) {
		return std::unexpected{Error{.what = "Empty glob expression"}};
	}

	auto const regex = glob::to_regex(expression);
	auto ret = Glob{};
	try {
		ret.m_regex = std::regex{regex};
	} catch (std::regex_error const& error) {
		return std::unexpected{Error{.what = error.what()}};
	}

	return ret;
}

auto Glob::is_exact_match(std::string_view const input) const -> bool {
	return std::regex_match(input.data(), input.data() + input.size(), m_regex);
}

auto Glob::is_partial_match(std::string_view const input) const -> bool {
	return std::regex_search(input.data(), input.data() + input.size(), m_regex);
}
} // namespace klib

#pragma endregion

#pragma region lerp_expr

namespace klib::lerp_expr {
void Token::Highlight::format_to(std::string& out, Token const& token, std::string_view const input_line) const {
	if (!out.empty()) {
		out.push_back('\n');
	}
	std::format_to(std::back_inserter(out), "{}{}\n{}{:>{}}", prefix, input_line, prefix, ' ', token.start_index);
	for (auto i = 0uz; i < token.lexeme.size(); ++i) {
		out.push_back(caret);
	}
}

auto Token::Highlight::format(Token const& token, std::string_view const input_line) const -> std::string {
	auto ret = std::string{};
	format_to(ret, token, input_line);
	return ret;
}
} // namespace klib::lerp_expr

#pragma endregion
