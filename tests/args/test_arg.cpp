#include "klib/unit_test.hpp"
#include <args/assigner.hpp>
#include <klib/args/arg.hpp>
#include <array>
#include <ranges>

namespace {
using namespace klib::args;

TEST(arg_flag) {
	bool flag{};
	auto const arg = named_flag(flag, "f,flag");
	auto const& param = arg.get_param();
	ASSERT(std::holds_alternative<ParamOption>(param));
	auto const& option = std::get<ParamOption>(param);
	EXPECT(option.is_flag);
	EXPECT(option.letter == 'f');
	EXPECT(option.word == "flag");
	EXPECT(flag == false);
	EXPECT(Assigner{}(option));
	EXPECT(flag == true);
}

TEST(arg_option_number) {
	int x{};
	auto arg = named_option(x, "x");
	auto param = arg.get_param();
	ASSERT(std::holds_alternative<ParamOption>(param));
	auto option = std::get<ParamOption>(param);
	EXPECT(!option.is_flag);
	EXPECT(option.letter == 'x');
	EXPECT(option.word.empty());
	EXPECT(x == 0);
	EXPECT(Assigner{"42"}(option));
	EXPECT(x == 42);
	EXPECT(!Assigner{"abc"}(option));

	float foo{};
	arg = named_option(foo, "foo");
	param = arg.get_param();
	ASSERT(std::holds_alternative<ParamOption>(param));
	option = std::get<ParamOption>(param);
	EXPECT(!option.is_flag);
	EXPECT(option.letter == '\0');
	EXPECT(option.word == "foo");
	EXPECT(foo == 0.0f);
	EXPECT(Assigner{"3.14"}(option));
	EXPECT(std::abs(foo - 3.14) < 0.001f);
	EXPECT(!Assigner{"abc"}(option));
}

TEST(arg_positional_number) {
	int x{};
	auto arg = positional_required(x, "x");
	auto param = arg.get_param();
	ASSERT(std::holds_alternative<ParamPositional>(param));
	auto positional = std::get<ParamPositional>(param);
	EXPECT(positional.is_required());
	EXPECT(positional.name == "x");
	EXPECT(Assigner{"42"}(positional) && x == 42);
	EXPECT(!Assigner{"abc"}(positional));

	float foo{};
	bool was_set{};
	arg = positional_optional(foo, "foo", {}, &was_set);
	param = arg.get_param();
	ASSERT(std::holds_alternative<ParamPositional>(param));
	positional = std::get<ParamPositional>(param);
	EXPECT(!positional.is_required());
	EXPECT(positional.name == "foo");
	EXPECT(Assigner{"3.14"}(positional) && std::abs(foo - 3.14) < 0.001f);
	EXPECT(was_set);
	was_set = false;
	EXPECT(!Assigner{"abc"}(positional));
	EXPECT(!was_set);
}

TEST(arg_string) {
	std::string_view foo{};
	auto arg = named_option(foo, "foo");
	auto param = arg.get_param();
	ASSERT(std::holds_alternative<ParamOption>(param));
	auto const& option = std::get<ParamOption>(param);
	EXPECT(!option.is_flag);
	EXPECT(option.letter == '\0');
	EXPECT(option.word == "foo");
	EXPECT(Assigner{"bar"}(option));
	EXPECT(foo == "bar");

	foo = {};
	bool was_set{};
	arg = positional_optional(foo, "foo", {}, &was_set);
	param = arg.get_param();
	ASSERT(std::holds_alternative<ParamPositional>(param));
	auto const& positional = std::get<ParamPositional>(param);
	EXPECT(!positional.is_required());
	EXPECT(positional.name == "foo");
	EXPECT(Assigner{"bar"}(positional) && foo == "bar");
	EXPECT(was_set);
}

TEST(arg_positional_list) {
	auto foo = std::vector<int>{};
	auto arg = positional_list(foo, "foo");
	auto param = arg.get_param();
	ASSERT(std::holds_alternative<ParamPositional>(param));
	auto const& positional = std::get<ParamPositional>(param);
	EXPECT(!positional.is_required());
	EXPECT(!positional.is_required());
	EXPECT(positional.name == "foo");
	EXPECT(Assigner{"1"}(positional));
	EXPECT(Assigner{"2"}(positional));
	EXPECT(!Assigner{"bar"}(positional));
	ASSERT(foo.size() == 2);
	EXPECT(foo[0] == 1 && foo[1] == 2);
}

TEST(arg_command) {
	struct CmdParams {
		bool verbose{};
		int number{};
	};
	auto params = CmdParams{};
	auto const args = std::array{
		named_flag(params.verbose, "v,verbose"),
		positional_required(params.number, "number"),
	};
	auto const cmd_arg = command(args, "cmd");
	auto param = cmd_arg.get_param();
	ASSERT(std::holds_alternative<ParamCommand>(param));
	auto const& cmd = std::get<ParamCommand>(param);
	EXPECT(cmd.name == "cmd");
	auto const cmd_args = std::span{cmd.arg_ptr, cmd.arg_count};
	for (auto const& [a, b] : std::ranges::zip_view(cmd_args, args)) { EXPECT(a.get_param().index() == b.get_param().index()); }
}
} // namespace
