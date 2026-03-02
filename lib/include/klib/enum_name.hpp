#pragma once
#include "klib/concepts.hpp"
#include <algorithm>
#include <optional>
#include <string_view>
#include <vector>

namespace klib {
template <EnumT E>
struct EnumNameMapping {
	E e{};
	std::string_view name{};
};

template <EnumT E, template <typename...> typename ContainerT = std::vector, typename... Args>
class EnumNameMap {
  public:
	using Entry = EnumNameMapping<E>;

	EnumNameMap() = default;

	explicit constexpr EnumNameMap(std::initializer_list<Entry> const& entries) : m_entries(entries) {}

	[[nodiscard]] constexpr auto to_name(E const e) const -> std::string_view {
		auto const pred = [e](Entry const& entry) { return entry.e == e; };
		if (auto const it = std::ranges::find_if(m_entries, pred); it != m_entries.end()) { return it->name; }
		return {};
	}

	[[nodiscard]] constexpr auto to_enum(std::string_view const name) const -> std::optional<E> {
		auto const pred = [name](Entry const& entry) { return entry.name == name; };
		if (auto const it = std::ranges::find_if(m_entries, pred); it != m_entries.end()) { return it->e; }
		return {};
	}

  private:
	ContainerT<Entry, Args...> m_entries{};
};
} // namespace klib
