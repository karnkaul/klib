module;

#include "klib/unit_test/unit_test.hpp"
#include <cstdlib>

export module klib.unit_test;

export import std;

namespace klib {
namespace unit_test {
export struct TestCase {
	virtual ~TestCase() = default;

	TestCase(TestCase const&) = delete;
	TestCase(TestCase&&) = delete;
	auto operator=(TestCase const&) = delete;
	auto operator=(TestCase&&) = delete;

	explicit TestCase(std::string_view name);

	virtual void run() const = 0;

	std::string_view name{};
};
}

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

		auto const pred = [&](unit_test::TestCase const* test_case) {
			if (star_start && star_end) { return !test_case->name.contains(pattern); }
			if (star_start) { return !test_case->name.ends_with(pattern); }
			return !test_case->name.starts_with(pattern);
		};
		std::erase_if(tests, pred);
	}

	std::vector<unit_test::TestCase*> tests{};
	bool failure{};
};

void check_failed(std::string_view const type, std::string_view const expr, std::string_view const file, int const line) {
	std::println(std::cerr, "{} failed: '{}' [{}:{}]", type, expr, file, line);
	State::self().failure = true;
}
} // namespace

namespace unit_test {
TestCase::TestCase(std::string_view const name) : name(name) { State::self().tests.push_back(this); }

export void check_expect(bool pred, std::string_view expr, std::string_view file, int line) {
	if (pred) { return; }
	check_failed("expectation", expr, file, line);
}

export void check_assert(bool pred, std::string_view expr, std::string_view file, int line) {
	if (pred) { return; }
	check_failed("assertion", expr, file, line);
	throw Assert{};
}

export auto run_tests(std::span<char const* const> args) -> int {
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
} // namespace unit_test
} // namespace klib
