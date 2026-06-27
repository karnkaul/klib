#pragma once

/// This header must be used in conjunction with module 'klib.unit_test'.

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
