#pragma once
#include "klib/base_types.hpp"
#include <string>
#include <string_view>

namespace klib {
class StringInterpolator : public Polymorphic {
  public:
	virtual void format_value_to(std::string& out, std::string_view identifier) const = 0;

	void interpolate_to(std::string& out, std::string_view input) const;

	[[nodiscard]] auto interpolate(std::string_view input) const -> std::string {
		auto ret = std::string{};
		interpolate_to(ret, input);
		return ret;
	}
};
} // namespace klib
