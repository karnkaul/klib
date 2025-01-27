#pragma once
#include <klib/args/binding.hpp>
#include <span>
#include <string_view>
#include <variant>

namespace klib::args {
class Arg;

struct Required {};
constexpr auto required_v = Required{};

using ArgType = std::variant<Required, bool*>;

struct ParamOption {
	Binding binding;
	void* data;
	bool* was_set;
	bool is_flag;
	char letter;
	std::string_view word;
	std::string_view help_text;

	[[nodiscard]] auto to_string() const -> std::string { return binding.to_string(data); }
};

struct ParamPositional {
	ArgType arg_type;
	Binding binding;
	void* data;
	bool is_list;
	std::string_view name;
	std::string_view help_text;

	[[nodiscard]] constexpr auto is_required() const -> bool { return std::holds_alternative<Required>(arg_type); }

	[[nodiscard]] auto to_string() const -> std::string { return binding.to_string(data); }
};

struct ParamCommand {
	Arg const* arg_ptr; // MSVC doesn't allow incomplete T in span<T>
	std::size_t arg_count;
	std::string_view name;
	std::string_view help_text;
};

using Param = std::variant<ParamOption, ParamPositional, ParamCommand>;

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
	// NOLINTNEXTLINE(readability-non-const-parameter)
	Arg(Type& out, ArgType const type, std::string_view const name, std::string_view const help_text = {})
		: m_param(ParamPositional{type, Binding::create<Type>(), &out, false, name, help_text}) {}

	template <ParamT Type>
	Arg(std::vector<Type>& out, std::string_view const name, std::string_view const help_text = {})
		: m_param(ParamPositional{nullptr, Binding::create<std::vector<Type>>(), &out, true, name, help_text}) {}

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
