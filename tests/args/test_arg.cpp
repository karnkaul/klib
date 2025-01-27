#include <args/assigner.hpp>
#include <klib/args/arg.hpp>
#include <klib/unit_test.hpp>
#include <ranges>

namespace {
using namespace klib::args;

TEST(arg_flag) {
	bool flag{};
	auto const arg = Arg{flag, "f,flag"};
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
	auto arg = Arg{x, "x"};
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
	arg = Arg{foo, "foo"};
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
	auto arg = Arg{x, ArgType::Required, "x"};
	auto param = arg.get_param();
	ASSERT(std::holds_alternative<ParamPositional>(param));
	auto positional = std::get<ParamPositional>(param);
	EXPECT(positional.arg_type == ArgType::Required);
	EXPECT(positional.name == "x");
	EXPECT(Assigner{"42"}(positional) && x == 42);
	EXPECT(!Assigner{"abc"}(positional));

	float foo{};
	arg = Arg{foo, ArgType::Optional, "foo"};
	param = arg.get_param();
	ASSERT(std::holds_alternative<ParamPositional>(param));
	positional = std::get<ParamPositional>(param);
	EXPECT(positional.arg_type == ArgType::Optional);
	EXPECT(positional.name == "foo");
	EXPECT(Assigner{"3.14"}(positional) && std::abs(foo - 3.14) < 0.001f);
	EXPECT(!Assigner{"abc"}(positional));
}

TEST(arg_string) {
	std::string_view foo{};
	auto arg = Arg{foo, "foo"};
	auto param = arg.get_param();
	ASSERT(std::holds_alternative<ParamOption>(param));
	auto const& option = std::get<ParamOption>(param);
	EXPECT(!option.is_flag);
	EXPECT(option.letter == '\0');
	EXPECT(option.word == "foo");
	EXPECT(Assigner{"bar"}(option));
	EXPECT(foo == "bar");

	foo = {};
	arg = Arg{foo, ArgType::Optional, "foo"};
	param = arg.get_param();
	ASSERT(std::holds_alternative<ParamPositional>(param));
	auto const& positional = std::get<ParamPositional>(param);
	EXPECT(positional.arg_type == ArgType::Optional);
	EXPECT(positional.name == "foo");
	EXPECT(Assigner{"bar"}(positional) && foo == "bar");
}

TEST(arg_command) {
	struct CmdParams {
		bool verbose{};
		int number{};
	};
	auto params = CmdParams{};
	auto const args = std::array{
		Arg{params.verbose, "v,verbose"},
		Arg{params.number, ArgType::Required, "number"},
	};
	auto const cmd_arg = Arg{args, "cmd"};
	auto param = cmd_arg.get_param();
	ASSERT(std::holds_alternative<ParamCommand>(param));
	auto const& cmd = std::get<ParamCommand>(param);
	EXPECT(cmd.name == "cmd");
	auto const cmd_args = std::span{cmd.arg_ptr, cmd.arg_count};
	for (auto const& [a, b] : std::ranges::zip_view(cmd_args, args)) { EXPECT(a.get_param().index() == b.get_param().index()); }
}
} // namespace
