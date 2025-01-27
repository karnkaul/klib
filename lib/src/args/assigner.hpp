#pragma once
#include <klib/args/arg.hpp>

namespace klib::args {
struct Assigner {
	std::string_view value;

	[[nodiscard]] auto assign(Binding const& binding, void* data, bool* was_set) const -> bool {
		auto const ret = binding.assign(data, value);
		if (ret && was_set != nullptr) { *was_set = true; }
		return ret;
	}

	auto operator()(ParamPositional const& positional) const -> bool { return assign(positional.binding, positional.data, positional.was_set); }
	auto operator()(ParamOption const& option) const -> bool { return assign(option.binding, option.data, option.was_set); }
	auto operator()(ParamCommand const& /*command*/) const -> bool { return false; }
};
} // namespace klib::args
