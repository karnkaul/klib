#pragma once

namespace klib {
template <typename CharT>
struct BasicFormatParser {
	template <typename FormatParseContext>
	constexpr auto parse(FormatParseContext& pc) const {
		return pc.begin();
	}
};

using FormatParser = BasicFormatParser<char>;
} // namespace klib
