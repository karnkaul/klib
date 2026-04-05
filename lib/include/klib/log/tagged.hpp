#pragma once
#include "klib/log/log.hpp"

namespace klib::log {
class Tagged {
  public:
	explicit Tagged(std::string_view const tag, Level const max_level = Level::Debug) : max_level(max_level), m_tag(tag) {}

	template <typename... Args>
	void error(Fmt<Args...> const& fmt, Args&&... args) const {
		log::error(m_tag, fmt, std::forward<Args>(args)...);
	}

	template <typename... Args>
	void warn(Fmt<Args...> const& fmt, Args&&... args) const {
		if (max_level < Level::Warn) { return; }
		log::warn(m_tag, fmt, std::forward<Args>(args)...);
	}

	template <typename... Args>
	void info(Fmt<Args...> const& fmt, Args&&... args) const {
		if (max_level < Level::Info) { return; }
		log::info(m_tag, fmt, std::forward<Args>(args)...);
	}

	template <typename... Args>
	void debug(Fmt<Args...> const& fmt, Args&&... args) const {
		if constexpr (!debug_enabled_v) { return; }
		if (max_level < Level::Debug) { return; }
		log::debug(m_tag, fmt, std::forward<Args>(args)...);
	}

	Level max_level{Level::Debug};

  private:
	std::string_view m_tag{};
};
} // namespace klib::log
