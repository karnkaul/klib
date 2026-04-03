#pragma once
#include "klib/log/log.hpp"

namespace klib::log {
class Tagged {
  public:
	explicit Tagged(std::string_view const tag) : m_tag(tag) {}

	template <typename... Args>
	void error(log::Fmt<Args...> const& fmt, Args&&... args) const {
		log::error(m_tag, fmt, std::forward<Args>(args)...);
	}

	template <typename... Args>
	void warn(log::Fmt<Args...> const& fmt, Args&&... args) const {
		log::warn(m_tag, fmt, std::forward<Args>(args)...);
	}

	template <typename... Args>
	void info(log::Fmt<Args...> const& fmt, Args&&... args) const {
		log::info(m_tag, fmt, std::forward<Args>(args)...);
	}

	template <typename... Args>
	void debug(log::Fmt<Args...> const& fmt, Args&&... args) const {
		if constexpr (!log::debug_enabled_v) { return; }
		log::debug(m_tag, fmt, std::forward<Args>(args)...);
	}

  private:
	std::string_view m_tag{};
};
} // namespace klib::log
