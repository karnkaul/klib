#pragma once
#include <expected>
#include <regex>
#include <string_view>

namespace klib {
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
