export module klib.string;

export import std;

#include "klib/concepts.hpp"

export namespace klib {
template <std::size_t MaxLength>
using StrBuf = std::array<char, MaxLength + 1>;

constexpr auto copy_to(std::span<char> out, std::string_view const text) -> std::size_t {
	auto const size = text.size() > out.size() ? out.size() : text.size();
	for (std::size_t i = 0; i < size; ++i) { out[i] = text[i]; }
	return size;
}

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

template <std::size_t MaxLength = 64>
class FixedString {
  public:
	FixedString() = default;

	template <std::convertible_to<std::string_view> T>
	explicit(false) constexpr FixedString(T const& text) : m_size(copy_to(m_buf, std::string_view{text})) {}

	template <typename... Args>
	explicit(false) FixedString(std::format_string<Args...> fmt, Args&&... args) {
		auto const [out, _] = std::format_to_n(m_buf.data(), std::iter_difference_t<char*>(MaxLength), fmt, std::forward<Args>(args)...);
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

	[[nodiscard]] constexpr auto substr(std::size_t const pos, std::size_t const count = std::string_view::npos) const -> FixedString {
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

template <typename CharT>
struct BasicFormatParser {
	template <typename FormatParseContext>
	constexpr auto parse(FormatParseContext& pc) const {
		return pc.begin();
	}
};

using FormatParser = BasicFormatParser<char>;

template <NumberT Type>
[[nodiscard]] auto str_to_num(std::string_view const str, Type const& fallback = {}) -> Type {
	auto ret = Type{};
	auto [_, ec] = std::from_chars(str.data(), str.data() + str.size(), ret);
	if (ec != std::errc{}) { return fallback; }
	return ret;
}

struct FromChars {
	template <NumberT Type>
	auto operator()(Type& out) -> bool {
		auto const* end = text.data() + text.size();
		auto const [ptr, ec] = std::from_chars(text.data(), end, out);
		if (ec != std::errc{}) { return false; }
		text = (ptr == end) ? std::string_view{} : std::string_view{ptr, std::size_t(end - ptr)};
		return true;
	}

	[[nodiscard]] auto advance_if(char const ch) -> bool {
		if (text.empty() || text.front() != ch) { return false; }
		text.remove_prefix(1);
		return true;
	}

	[[nodiscard]] auto advance_if_any(std::string_view chars) -> bool {
		if (chars.empty() || text.empty()) { return false; }
		return std::ranges::any_of(chars, [this](char const ch) { return advance_if(ch); });
	}

	[[nodiscard]] auto advance_if_all(std::string_view str) -> bool {
		if (text.empty() || str.empty() || !text.starts_with(str)) { return false; }
		text.remove_prefix(str.size());
		return true;
	}

	std::string_view text{};
};
} // namespace klib

namespace klib::escape {
export using Rgb = std::array<std::uint8_t, 3>;

export constexpr auto prefix_v = std::string_view{"\x1b["};
export constexpr auto suffix_v = std::string_view{"m"};

namespace {
[[nodiscard]] auto colorify(Rgb const rgb, int const target) -> FixedString<> {
	return {"{}{};2;{};{};{}{}", prefix_v, target, rgb[0], rgb[1], rgb[2], suffix_v};
}
} // namespace

export auto const clear = FixedString<>{"{}{}", prefix_v, suffix_v};

export [[nodiscard]] auto foreground(Rgb const rgb) -> FixedString<> { return colorify(rgb, 38); }
export [[nodiscard]] auto background(Rgb const rgb) -> FixedString<> { return colorify(rgb, 48); }
} // namespace klib::escape

template <>
struct std::formatter<klib::CString> : formatter<string_view> {
	template <typename FormatContext>
	auto format(klib::CString const& str, FormatContext& fc) const {
		return formatter<string_view>::format(str.as_view(), fc);
	}
};

template <std::size_t N>
struct std::formatter<klib::FixedString<N>> : formatter<string_view> {
	template <typename FormatContext>
	auto format(klib::FixedString<N> const& str, FormatContext& fc) const {
		return formatter<string_view>::format(str.as_view(), fc);
	}
};
