#include "klib/log/file.hpp"
#include "klib/log/typed.hpp"
#include "klib/string/c_string.hpp"
#include "klib/unit_test/unit_test.hpp"
#include "util.hpp"
#include <filesystem>
#include <fstream>
#include <print>

namespace {
using namespace klib;

struct LogTestType {};

TEST_CASE(log) {
	static constexpr CString filename_v{"test.log"};
	auto const test_dir = TestDir{};
	auto const path = test_dir.to_path(filename_v.as_view()).string();

	auto const logger = log::Typed<LogTestType>{};
	{
		auto const file = log::File{path};
		logger.info("expect in log file");
	}
	logger.info("unexpected in log file");

	logger.error("test error log");
	logger.warn("test warn log");
	logger.debug("test debug log");

	{
		auto file = std::ifstream{path};
		ASSERT(file.is_open());
		std::println("{} contents:", filename_v.as_view());
		auto line = std::string{};
		EXPECT(std::getline(file, line));
		EXPECT(line.contains("expect in log file"));
		if (!line.empty()) { std::println("{}", line); }
		EXPECT(!std::getline(file, line));
		if (!line.empty()) { std::println("{}", line); }
	}
}
} // namespace
