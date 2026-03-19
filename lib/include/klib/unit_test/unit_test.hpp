#pragma once
#include <span>
#include <string_view>

namespace klib::unit_test {
void check_expect(bool pred, std::string_view expr, std::string_view file, int line);
void check_assert(bool pred, std::string_view expr, std::string_view file, int line);

using TestFunc = void (*)();

struct TestCase {
	virtual ~TestCase() = default;

	TestCase(TestCase const&) = delete;
	TestCase(TestCase&&) = delete;
	auto operator=(TestCase const&) = delete;
	auto operator=(TestCase&&) = delete;

	explicit TestCase(std::string_view name);

	virtual void run() const = 0;

	std::string_view name{};
};

auto run_tests(std::span<char const* const> args) -> int;
} // namespace klib::unit_test

#define ASSERT(pred) ::klib::unit_test::check_assert(bool(pred), #pred, __FILE__, __LINE__) // NOLINT(cppcoreguidelines-macro-usage)
#define EXPECT(pred) ::klib::unit_test::check_expect(bool(pred), #pred, __FILE__, __LINE__) // NOLINT(cppcoreguidelines-macro-usage)

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define TEST_CASE(name)                                                                                                                                        \
	struct TestCase_##name : ::klib::unit_test::TestCase {                                                                                                     \
		using TestCase::TestCase;                                                                                                                              \
		void run() const final;                                                                                                                                \
	};                                                                                                                                                         \
	TestCase_##name const g_test_case_##name{#name};                                                                                                           \
	void TestCase_##name::run() const
