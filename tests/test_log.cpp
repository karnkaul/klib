#include <klib/log_file.hpp>
#include <klib/unit_test.hpp>
#include <filesystem>
#include <fstream>
#include <future>
#include <mutex>
#include <vector>

namespace {
namespace fs = std::filesystem;

using namespace klib;

constexpr auto trim(std::string_view line) {
	while (!line.empty() && (line.back() == '\n' || line.back() == '\r')) { line = line.substr(0, line.size() - 1); }
	return line;
}

struct Sink : log::ISink {
	void on_log(log::Input const& /*input*/, CString text) final {
		auto lock = std::scoped_lock{m_mutex};
		lines.emplace_back(trim(text.as_view()));
	}

	std::vector<std::string> lines{};

  private:
	std::mutex m_mutex{};
};

TEST(log_file) {
	static constexpr CString filename_v{"klib_test.log"};
	static constexpr std::string_view tag_v{"test"};
	auto sink = std::make_shared<Sink>();
	auto file_sink = std::make_shared<log::FileSink>(filename_v);

	log::attach(sink);
	log::attach(file_sink);

	static auto const log_line = [](log::Level const level) {
		switch (level) {
		case log::Level::Error: log::error(tag_v, "test log at level '{}'", log::level_to_char[level]); break;
		case log::Level::Warn: log::warn(tag_v, "test log at level '{}'", log::level_to_char[level]); break;
		case log::Level::Info: log::info(tag_v, "test log at level '{}'", log::level_to_char[level]); break;
		case log::Level::Debug: log::debug(tag_v, "test log at level '{}'", log::level_to_char[level]); break;
		default: break;
		}
	};

	auto futures = std::vector<std::future<void>>{};
	futures.reserve(4);

	using Level_t = std::underlying_type_t<log::Level>;
	auto level = Level_t(log::Level::Error);
	log_line(log::Level{level++});
	while (level < Level_t(log::Level::COUNT_)) {
		futures.push_back(std::async([level = log::Level(level)] { log_line(level); }));
		++level;
	}

	futures.clear();
	auto lines = std::span{sink->lines};
	EXPECT(!lines.empty());

	file_sink.reset();

	auto file = std::ifstream{filename_v.c_str()};
	EXPECT(file.is_open());
	for (auto line = std::string{}; std::getline(file, line);) {
		ASSERT(!lines.empty());
		EXPECT(line == lines.front());
		lines = lines.subspan(1);
	}
	file.close();

	auto ec = std::error_code{};
	EXPECT(fs::remove(filename_v.as_view(), ec));
}
} // namespace
