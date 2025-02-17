#pragma once
#include <klib/args/binding.hpp>
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
} // namespace klib::args
