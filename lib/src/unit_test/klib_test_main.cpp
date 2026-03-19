#include "klib/build_version.hpp"
#include "klib/unit_test/unit_test.hpp"
#include <print>

auto main(int argc, char** argv) -> int {
	try {
		auto args = std::span{argv, std::size_t(argc)};
		if (!args.empty()) { args = args.subspan(1); }
		std::println("- klib/unit_test {} -", klib::build_version_v);
		return klib::unit_test::run_tests(args);
	} catch (std::exception const& e) {
		std::println(stderr, "PANIC: {}", e.what());
		return EXIT_FAILURE;
	} catch (...) {
		std::println(stderr, "PANIC");
		return EXIT_FAILURE;
	}
}
