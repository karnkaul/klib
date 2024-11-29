#include <klib/unit_test.hpp>
#include <print>

auto main() -> int {
	try {
		return klib::test::run_tests();
	} catch (std::exception const& e) {
		std::println(stderr, "PANIC: {}", e.what());
		return EXIT_FAILURE;
	} catch (...) {
		std::println(stderr, "PANIC");
		return EXIT_FAILURE;
	}
}
