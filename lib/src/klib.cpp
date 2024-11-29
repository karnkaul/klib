#include <print>
#include <sstream>
#include <vector>

// unit_test

#include <klib/unit_test.hpp>

namespace klib {
namespace {
struct Assert {};

struct State {
	std::vector<test::TestCase*> tests{};
	bool failure{};

	static auto self() -> State& {
		static auto ret = State{};
		return ret;
	}
};

void check_failed(std::string_view const type, std::string_view const expr, std::string_view const file, int const line) {
	std::println(stderr, "{} failed: '{}' [{}:{}]", type, expr, file, line);
	State::self().failure = true;
}
} // namespace

void test::check_expect(bool const pred, std::string_view const expr, std::string_view const file, int const line) {
	if (pred) { return; }
	check_failed("expectation", expr, file, line);
}

void test::check_assert(bool const pred, std::string_view const expr, std::string_view const file, int const line) {
	if (pred) { return; }
	check_failed("assertion", expr, file, line);
	throw Assert{};
}

auto test::run_tests() -> int {
	auto& state = State::self();
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

namespace test {
TestCase::TestCase(std::string_view const name) : name(name) { State::self().tests.push_back(this); }
} // namespace test
} // namespace klib

// version

#include <klib/version_str.hpp>

void klib::append_to(std::string& out, Version const& version) {
	std::format_to(std::back_inserter(out), "v{}.{}.{}", version.major, version.minor, version.patch);
}

auto klib::to_string(Version const& version) -> std::string {
	auto ret = std::string{};
	append_to(ret, version);
	return ret;
}

auto klib::to_version(CString text) -> Version {
	if (text.as_view().starts_with('v')) { text = CString{text.as_view().substr(1).data()}; }
	if (text.as_view().empty()) { return {}; }

	auto ret = Version{};
	auto discard = char{'.'};
	auto str = std::istringstream{text.c_str()};
	str >> ret.major >> discard >> ret.minor >> discard >> ret.patch;
	return ret;
}
