export module klib.unit_test;

export import std;

export namespace klib::unit_test {
void check_expect(bool pred, std::string_view expr, std::string_view file, int line);
void check_assert(bool pred, std::string_view expr, std::string_view file, int line);

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
