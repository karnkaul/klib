#include "klib/unit_test/unit_test.hpp"
#include <cstdlib>
#include <print>
#include <string_view>
#include <vector>

namespace klib {
namespace unit_test {
namespace {
struct Assert {};

struct State {
	static auto self() -> State& {
		static auto ret = State{};
		return ret;
	}

	void filter_matches(std::string_view pattern) {
		auto const star_start = pattern.starts_with('*');
		auto const star_end = pattern.ends_with('*');
		if (star_start) { pattern.remove_prefix(1); }
		if (star_end) { pattern.remove_suffix(1); }

		auto const pred = [&](TestCase const* test_case) {
			if (star_start && star_end) { return !test_case->name.contains(pattern); }
			if (star_start) { return !test_case->name.ends_with(pattern); }
			return !test_case->name.starts_with(pattern);
		};
		std::erase_if(tests, pred);
	}

	std::vector<TestCase*> tests{};
	bool failure{};
};

void check_failed(std::string_view const type, std::string_view const expr, std::string_view const file, int const line) {
	std::println(stderr, "{} failed: '{}' [{}:{}]", type, expr, file, line);
	State::self().failure = true;
}
} // namespace

TestCase::TestCase(std::string_view const name) : name(name) { State::self().tests.push_back(this); }
} // namespace unit_test

void unit_test::check_expect(bool const pred, std::string_view const expr, std::string_view const file, int const line) {
	if (pred) { return; }
	check_failed("expectation", expr, file, line);
}

void unit_test::check_assert(bool const pred, std::string_view const expr, std::string_view const file, int const line) {
	if (pred) { return; }
	check_failed("assertion", expr, file, line);
	throw Assert{};
}

auto unit_test::run_tests(std::span<char const* const> args) -> int {
	auto& state = State::self();
	while (!args.empty()) {
		state.filter_matches(args.front());
		args = args.subspan(1);
	}

	for (auto* test : state.tests) {
		try {
			std::println("[{}]", test->name);
			test->run();
		} catch (Assert const& /*a*/) {}
	}

	if (state.failure) {
		std::println("FAILED");
		return EXIT_FAILURE;
	}

	std::println("passed");
	return EXIT_SUCCESS;
}
} // namespace klib
