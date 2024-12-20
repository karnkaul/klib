#pragma once

namespace klib {
template <typename... Args>
struct Noop {
	constexpr void operator()(Args&&... /*unused*/) const noexcept {}
};
} // namespace klib
