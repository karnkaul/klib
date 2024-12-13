#include <klib/log.hpp>
#include <klib/unit_test.hpp>
#include <filesystem>
#include <fstream>
#include <print>

namespace {
using namespace klib;

struct Logger {
	std::string_view tag{"klib::test"};

	template <typename... Args>
	void info(log::Fmt<Args...> fmt, Args&&... args) const {
		log::info(tag, fmt, std::forward<Args>(args)...);
	}
};

TEST(log) {
	static constexpr CString filename_v{"test.log"};

	auto const logger = Logger{};
	{
		auto const file = log::File{filename_v.c_str()};
		logger.info("expect in log file");
	}
	logger.info("unexpected in log file");

	{
		auto file = std::ifstream{filename_v.c_str()};
		ASSERT(file.is_open());
		std::println("{} contents:", filename_v.as_view());
		auto line = std::string{};
		EXPECT(std::getline(file, line));
		EXPECT(line.contains("expect in log file"));
		if (!line.empty()) { std::println("{}", line); }
		EXPECT(!std::getline(file, line));
		if (!line.empty()) { std::println("{}", line); }
	}

	auto ec = std::error_code{};
	EXPECT(std::filesystem::remove("test.log", ec));
}
} // namespace
