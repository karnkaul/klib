#include <klib/build_version.hpp>
#include <klib/unit_test.hpp>
#include <klib/version_str.hpp>
#include <print>

auto main() -> int {
	try {
		std::println("- klib/unit_test {} -", klib::build_version_v);
		return klib::test::run_tests();
	} catch (std::exception const& e) {
		std::println(stderr, "PANIC: {}", e.what());
		return EXIT_FAILURE;
	} catch (...) {
		std::println(stderr, "PANIC");
		return EXIT_FAILURE;
	}
}
