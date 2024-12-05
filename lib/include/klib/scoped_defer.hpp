#pragma once
#include <klib/unique.hpp>
#include <functional>

namespace klib {
class ScopedDefer {
  public:
	using Func = std::move_only_function<void()>;

	template <std::convertible_to<Func> F = Func>
	/*implicit*/ ScopedDefer(F func = {}) : m_func(std::move(func)) {}

  private:
	struct Deleter {
		void operator()(Func func) const noexcept {
			if (!func) { return; }
			func();
		}
	};
	Unique<Func, Deleter> m_func;
};
} // namespace klib
