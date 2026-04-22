#pragma once
#include "klib/concepts.hpp"
#include <algorithm>
#include <optional>
#include <span>
#include <string_view>
#include <utility>
#include <vector>

namespace klib {
template <EnumT E, template <typename...> typename ContainerT = std::vector, typename... Args>
class EnumNameMap {
  public:
	using Entry = std::pair<E, std::string_view>;

	EnumNameMap() = default;

	explicit constexpr EnumNameMap(std::initializer_list<Entry> const& entries) : m_entries(entries) {}

	[[nodiscard]] constexpr auto to_name(E const e) const -> std::string_view {
		auto const pred = [e](Entry const& entry) { return entry.first == e; };
		if (auto const it = std::ranges::find_if(m_entries, pred); it != m_entries.end()) { return it->second; }
		return {};
	}

	[[nodiscard]] constexpr auto to_enum(std::string_view const name) const -> std::optional<E> {
		auto const pred = [name](Entry const& entry) { return entry.second == name; };
		if (auto const it = std::ranges::find_if(m_entries, pred); it != m_entries.end()) { return it->first; }
		return {};
	}

	[[nodiscard]] constexpr auto as_span() const -> std::span<Entry const> { return m_entries; }
	[[nodiscard]] constexpr auto as_span() -> std::span<Entry> { return m_entries; }

  private:
	ContainerT<Entry, Args...> m_entries{};
};
} // namespace klib
