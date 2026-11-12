module;

#if __has_include(<cxxabi.h>)
#define KLIB_USE_CXA_DEMANGLE
#include <cxxabi.h>
#endif

export module klib.core.string;

export import std;

#pragma region FormatParser

export namespace klib {
template <typename CharT>
struct BasicFormatParser {
	template <typename FormatParseContext>
	constexpr auto parse(FormatParseContext& pc) const {
		return pc.begin();
	}
};

using FormatParser = BasicFormatParser<char>;
} // namespace klib

#pragma endregion

#pragma region CString

export namespace klib {
/// \brief Wrapper over a C string.
/// Never points to nullptr.
class CString {
  public:
	CString() = default;

	CString(std::nullptr_t) = delete;

	template <std::size_t N>
	// NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays,modernize-avoid-c-arrays)
	explicit(false) constexpr CString(char const (&str)[N]) : m_str(str, N) {}

	explicit(false) constexpr CString(char const* str) : m_str(str ? str : "") {}

	template <std::same_as<std::string> T>
	explicit(false) constexpr CString(T const& str) : m_str(str) {}

	[[nodiscard]] constexpr auto as_view() const -> std::string_view { return m_str; }
	[[nodiscard]] constexpr auto c_str() const -> char const* { return m_str.data(); }

	auto operator<=>(CString const&) const = default;

  private:
	// NOLINTNEXTLINE(readability-redundant-string-init)
	std::string_view m_str{""};
};
} // namespace klib

template <>
struct std::formatter<klib::CString> : formatter<string_view> {
	template <typename FormatContext>
	auto format(klib::CString const& str, FormatContext& fc) const {
		return formatter<string_view>::format(str.as_view(), fc);
	}
};

#pragma endregion

#pragma region StrBuf

export namespace klib {
template <std::size_t MaxLength>
using StrBuf = std::array<char, MaxLength + 1>;

constexpr auto copy_to(std::span<char> out, std::string_view const text) -> std::size_t {
	auto ret = 0uz;
	for (auto [o, i] : std::views::zip(out, text)) {
		o = i;
		++ret;
	}
	return ret;
}
} // namespace klib

#pragma endregion

#pragma region FixedString

export namespace klib {
template <std::size_t MaxLength = 64>
class FixedString {
  public:
	FixedString() = default;

	template <std::convertible_to<std::string_view> T>
	explicit(false) constexpr FixedString(T const& text) : m_size(copy_to(m_buf, std::string_view{text})) {}

	template <typename... Args>
	explicit(false) FixedString(std::format_string<Args...> fmt, Args&&... args) {
		auto const [out, _] = std::format_to_n(
			m_buf.data(), static_cast<std::iter_difference_t<char*>>(MaxLength), fmt, std::forward<Args>(args)...);
		m_size = std::size_t(out - m_buf.data()); // NOLINT(cppcoreguidelines-prefer-member-initializer)
	}

	template <std::convertible_to<std::string_view> T>
	constexpr auto append(T const& rhs) -> FixedString& {
		auto const remain = std::span<char>{m_buf}.subspan(m_size);
		m_size += copy_to(remain, std::string_view{rhs});
		return *this;
	}

	[[nodiscard]] constexpr auto is_empty() const -> bool { return m_size == 0; }

	constexpr void clear() {
		m_buf = {};
		m_size = 0;
	}

	constexpr auto substr(std::size_t const pos, std::size_t const count = std::string_view::npos) const
		-> FixedString {
		return FixedString{as_view().substr(pos, count)};
	}

	[[nodiscard]] constexpr auto data() const -> char const* { return m_buf.data(); }
	[[nodiscard]] constexpr auto c_str() const -> char const* { return data(); }
	[[nodiscard]] constexpr auto as_view() const -> std::string_view { return std::string_view{data(), m_size}; }

	template <std::convertible_to<std::string_view> T>
	constexpr auto operator+=(T const& rhs) -> FixedString& {
		return append(rhs);
	}

	template <std::size_t N>
	constexpr auto operator==(FixedString<N> const& rhs) const -> bool {
		return as_view() == rhs.as_view();
	}

	constexpr auto operator==(std::string_view const rhs) const -> bool { return as_view() == rhs; }

	constexpr operator std::string_view() const { return as_view(); }

  private:
	StrBuf<MaxLength> m_buf{};
	std::size_t m_size{};
};
} // namespace klib

template <std::size_t N>
struct std::formatter<klib::FixedString<N>> : formatter<string_view> {
	template <typename FormatContext>
	auto format(klib::FixedString<N> const& str, FormatContext& fc) const {
		return formatter<string_view>::format(str.as_view(), fc);
	}
};

#pragma endregion

#pragma region TextTable

export namespace klib {
class TextTable {
  public:
	enum class Align : std::int8_t { Left, Center, Right };

	class Builder;

	void push_row(std::vector<std::string> row);
	void push_separator() { push_row(separator()); }

	void append_to(std::string& out) const;
	[[nodiscard]] auto serialize() const -> std::string;

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

export namespace klib::escape {
inline constexpr auto prefix_v = std::string_view{"\x1b["};
inline constexpr auto suffix_v = std::string_view{"m"};

using Rgb = std::array<std::uint8_t, 3>;

inline auto const clear = FixedString<>{"{}{}", prefix_v, suffix_v};

[[nodiscard]] auto foreground(Rgb rgb) -> FixedString<>;
[[nodiscard]] auto background(Rgb rgb) -> FixedString<>;
} // namespace klib::escape

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

namespace klib::detail {
template <typename Type, typename... Args>
auto do_from_chars(std::string_view& out_text, Type& out_number, Args const... args) -> bool {
	auto const* end = out_text.data() + out_text.size();
	auto const [ptr, ec] = std::from_chars(out_text.data(), end, out_number, args...);
	if (ec != std::errc{}) {
		return false;
	}
	out_text = (ptr == end) ? std::string_view{} : std::string_view{ptr, std::size_t(end - ptr)};
	return true;
}
} // namespace klib::detail

export namespace klib {
template <typename Type>
concept NumberT = !std::same_as<bool, Type> && (std::integral<Type> || std::floating_point<Type>);

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
	if (!FromChars{.text = text}(ret)) {
		return fallback;
	}
	return ret;
}
} // namespace klib

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

export namespace klib {
[[nodiscard]] auto demangled_name(std::type_index index) -> std::string;
[[nodiscard]] auto demangled_name(std::type_info const& info) -> std::string;

template <typename Type>
[[nodiscard]] auto demangled_name() -> std::string const& {
	static std::string const ret = [] {
		return demangled_name(typeid(Type));
	}();
	return ret;
}

template <typename Type>
	requires(std::is_polymorphic_v<Type>)
[[nodiscard]] auto demangled_name(Type const& t) -> std::string {
	return demangled_name(typeid(t));
}
} // namespace klib

namespace {
auto demangled_name_impl(char const* type_name) -> std::string {
#ifdef KLIB_USE_CXA_DEMANGLE
	auto status = int{};
	auto const buf = std::unique_ptr<char, decltype(std::free)*>{
		abi::__cxa_demangle(type_name, nullptr, nullptr, &status),
		std::free,
	};
	if (status != 0) {
		return type_name;
	}
	auto ret = std::string_view{buf.get()};
	if (auto const i = ret.find('@'); i != std::string_view::npos) {
		ret = ret.substr(0, i);
	}
	return std::string{ret};
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

export namespace klib {
class Glob {
  public:
	struct Error {
		std::string what{};
	};

	[[nodiscard]] static auto create(std::string_view expression) -> std::expected<Glob, Error>;

	[[nodiscard]] auto is_exact_match(std::string_view input) const -> bool;
	[[nodiscard]] auto is_partial_match(std::string_view input) const -> bool;

  private:
	std::regex m_regex{};
};
} // namespace klib

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

export namespace klib::lerp_expr {
struct Token {
	struct Highlight;

	enum class Type : std::int8_t { String, Identifier };

	Type type{};
	std::string_view lexeme{};
	std::size_t start_index{};
};

struct Token::Highlight {
	void format_to(std::string& out, Token const& token, std::string_view input_line) const;
	[[nodiscard]] auto format(Token const& token, std::string_view input_line) const -> std::string;

	std::string_view prefix{" | "};
	char caret{'^'};
};

template <typename FuncT>
concept FormatIdentifierT = requires(FuncT func, std::string& out, Token const& identifier) {
	{ func(out, identifier) };
};

template <FormatIdentifierT FuncT>
constexpr void interpolate_to(std::string& out, std::string_view const input, FuncT format_identifier) {
	auto const per_token = [&](Token const& token) {
		switch (token.type) {
		case Token::Type::String: out.append(token.lexeme); break;
		case Token::Type::Identifier: format_identifier(out, token); break;
		default: break;
		}
	};
	tokenize(input, per_token);
}

template <FormatIdentifierT FuncT>
[[nodiscard]] constexpr auto interpolate(std::string_view const input, FuncT format_identifier) -> std::string {
	auto ret = std::string{};
	interpolate_to(ret, input, std::move(format_identifier));
	return ret;
}

template <typename FuncT>
concept PerTokenT = requires(FuncT func, Token const& token) {
	{ func(token) };
};

class Scanner {
  public:
	static constexpr std::string_view enclose_v = "{}";

	using Type = Token::Type;

	explicit constexpr Scanner(std::string_view const text) : m_text(text) {}

	constexpr auto scan_next(Token& out) -> bool {
		if (m_index >= m_text.size()) {
			return false;
		}

		// NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)
		auto const i = std::min(m_text.find(enclose_v[0], m_index), m_text.size());
		if (i == m_index) {
			out = to_identifier();
			return true;
		}

		out = to_token(Type::String, i - m_index);
		return true;
	}

  private:
	[[nodiscard]] constexpr auto to_identifier() -> Token {
		++m_index;

		// NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)
		auto const i = m_text.find(enclose_v[1], m_index);
		if (i == std::string_view::npos) {
			return to_token(Type::Identifier, m_text.size());
		}

		auto const ret = to_token(Type::Identifier, i - m_index);
		++m_index;

		return ret;
	}

	[[nodiscard]] constexpr auto to_token(Type const type, std::size_t length) -> Token {
		length = std::min(length, m_text.size() - m_index);
		auto const ret = Token{
			.type = type,
			.lexeme = m_text.substr(m_index, length),
			.start_index = m_index,
		};
		m_index += length;
		return ret;
	}

	std::string_view m_text{};
	std::size_t m_index{};
};

template <PerTokenT Func>
constexpr void tokenize(std::string_view const text, Func per_token) {
	auto scanner = Scanner{text};
	auto token = Token{};
	while (scanner.scan_next(token)) {
		per_token(token);
	}
}
} // namespace klib::lerp_expr

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
