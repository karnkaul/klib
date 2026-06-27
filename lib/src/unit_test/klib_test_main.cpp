#include <cstdlib>

import std;
import klib.unit_test;
import klib.build_version;

auto main(int argc, char** argv) -> int {
	try {
		auto args = std::span{argv, std::size_t(argc)};
		if (!args.empty()) { args = args.subspan(1); }
		std::println("- klib/unit_test {} -", klib::build_version_v);
		return klib::unit_test::run_tests(args);
	} catch (std::exception const& e) {
		std::println(std::cerr, "PANIC: {}", e.what());
		return EXIT_FAILURE;
	} catch (...) {
		std::println(std::cerr, "PANIC");
		return EXIT_FAILURE;
	}
}
