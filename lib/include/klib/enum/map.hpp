#pragma once
#include "klib/concepts.hpp"
#include "klib/ptr.hpp"
#include <algorithm>
#include <optional>
#include <span>
#include <string_view>
#include <utility>
#include <vector>

namespace klib {
template <EnumT E, typename ValueT, template <typename...> typename ContainerT = std::vector, typename... Args>
class EnumMap {
  public:
	using Entry = std::pair<E, ValueT>;

	EnumMap() = default;

	explicit constexpr EnumMap(std::initializer_list<Entry> const& entries) : m_entries(entries) {}

	[[nodiscard]] constexpr auto to_value(E const e) const -> Ptr<ValueT const> {
		auto const pred = [e](Entry const& entry) { return entry.first == e; };
		if (auto const it = std::ranges::find_if(m_entries, pred); it != m_entries.end()) { return &it->second; }
		return nullptr;
	}

	[[nodiscard]] constexpr auto to_enum(ValueT const& value) const -> std::optional<E>
		requires(std::equality_comparable<ValueT>)
	{
		auto const pred = [&value](Entry const& entry) { return entry.second == value; };
		if (auto const it = std::ranges::find_if(m_entries, pred); it != m_entries.end()) { return it->first; }
		return {};
	}

	[[nodiscard]] constexpr auto as_span() const -> std::span<Entry const> { return m_entries; }
	[[nodiscard]] constexpr auto as_span() -> std::span<Entry> { return m_entries; }

  private:
	ContainerT<Entry, Args...> m_entries{};
};

template <EnumT E, template <typename...> typename ContainerT = std::vector, typename... Args>
class EnumNameMap : public EnumMap<E, std::string_view, std::vector, Args...> {
  public:
	using Entry = EnumMap<E, std::string_view>::Entry;

	using EnumMap<E, std::string_view>::EnumMap;

	[[nodiscard]] constexpr auto to_name(E const e, std::string_view const fallback = {}) const -> std::string_view {
		if (auto const ptr = this->to_value(e)) { return *ptr; }
		return fallback;
	}
};
} // namespace klib
