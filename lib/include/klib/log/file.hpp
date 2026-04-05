#pragma once
#include <string>

namespace klib::log {
class File {
  public:
	File(File const&) = delete;
	File(File&&) = delete;
	auto operator=(File const&) = delete;
	auto operator=(File&&) = delete;

	explicit File(std::string path = "debug.log");
	~File();

	[[nodiscard]] auto is_attached() const -> bool;
	[[nodiscard]] auto get_path() const -> std::string_view { return m_path; }

  private:
	std::string m_path;
};
} // namespace klib::log
