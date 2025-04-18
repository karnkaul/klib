#pragma once

namespace klib {
template <typename CharT>
struct BasicFormatParser {
	template <typename FormatParseContext>
	constexpr auto parse(FormatParseContext& pc) {
		return pc.end();
	}
};

using FormatParser = BasicFormatParser<char>;
} // namespace klib
