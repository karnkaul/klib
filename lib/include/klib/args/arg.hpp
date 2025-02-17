#pragma once
#include <klib/args/binding.hpp>
#include <klib/args/param.hpp>
#include <span>
#include <string_view>

namespace klib::args {
class Arg {
  public:
	// Named options
	Arg(bool& out, std::string_view key, std::string_view help_text = {}, bool* was_set = {});

	template <ParamT Type>
	// NOLINTNEXTLINE(readability-non-const-parameter)
	Arg(Type& out, std::string_view const key, std::string_view const help_text = {}, bool* was_set = {})
		: m_param(ParamOption{Binding::create<Type>(), &out, was_set, false, to_letter(key), to_word(key), help_text}) {}

	// Positional arguments
	template <ParamT Type>
	Arg(Type& out, ArgType const type, std::string_view const name, std::string_view const help_text = {})
		: m_param(ParamPositional{type, Binding::create<Type>(), &out, false, name, help_text}) {}

	template <ParamT Type>
	Arg(std::vector<Type>& out, std::string_view const name, std::string_view const help_text = {})
		: m_param(ParamPositional{ArgType{nullptr}, Binding::create<std::vector<Type>>(), &out, true, name, help_text}) {}

	// Commands
	Arg(std::span<Arg const> args, std::string_view name, std::string_view help_text = {});

	[[nodiscard]] auto get_param() const -> Param const& { return m_param; }

	static constexpr auto to_letter(std::string_view const key) -> char {
		if (key.size() == 1 || (key.size() > 2 && key[1] == ',')) { return key.front(); }
		return '\0';
	}

	static constexpr auto to_word(std::string_view const key) -> std::string_view {
		if (key.size() > 1) {
			if (key[1] == ',') { return key.substr(2); }
			return key;
		}
		return {};
	}

  private:
	Param m_param;
};

[[nodiscard]] inline auto named_flag(bool& out, std::string_view const key, std::string_view const help_text = {}, bool* was_set = {}) -> Arg {
	return {out, key, help_text, was_set};
}

template <ParamT Type>
[[nodiscard]] auto named_option(Type& out, std::string_view const key, std::string_view const help_text = {}, bool* was_set = {}) -> Arg {
	return {out, key, help_text, was_set};
}

template <ParamT Type>
[[nodiscard]] auto positional_required(Type& out, std::string_view const name, std::string_view const help_text = {}) -> Arg {
	return {out, required_v, name, help_text};
}

template <ParamT Type>
[[nodiscard]] auto positional_optional(Type& out, std::string_view const name, std::string_view const help_text = {}, bool* was_set = {}) -> Arg {
	return {out, was_set, name, help_text};
}

template <ParamT Type>
[[nodiscard]] auto positional_list(std::vector<Type>& out, std::string_view const name, std::string_view const help_text = {}) -> Arg {
	return {out, name, help_text};
}

[[nodiscard]] inline auto command(std::span<Arg const> args, std::string_view name, std::string_view help_text = {}) -> Arg { return {args, name, help_text}; }
} // namespace klib::args
