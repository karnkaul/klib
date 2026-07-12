#pragma once
#include <filesystem>

namespace klib {
namespace fs = std::filesystem;

class TestDir {
  public:
	TestDir(TestDir const&) = delete;
	TestDir(TestDir&&) = delete;
	TestDir& operator=(TestDir const&) = delete;
	TestDir& operator=(TestDir&&) = delete;

	explicit TestDir(fs::path path = "test_dir") : m_path(std::move(path)) {
		if (!fs::exists(m_path)) { fs::create_directories(m_path); }
	}

	~TestDir() { fs::remove_all(m_path); }

	[[nodiscard]] auto get_path() const -> fs::path const& { return m_path; }
	[[nodiscard]] auto to_path(std::string_view const uri) const -> fs::path { return m_path / uri; }

  private:
	fs::path m_path{};
};
} // namespace klib
