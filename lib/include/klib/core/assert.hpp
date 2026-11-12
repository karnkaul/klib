#pragma once
#include "klib/core/constants.hpp"
#include <concepts>
#include <cstdint>
#include <exception>
#include <stacktrace>
#include <string>

namespace klib::assertion {
enum class FailAction : std::uint8_t { Throw, Terminate, None };

struct Failure : std::exception {
	[[nodiscard]] auto what() const noexcept -> char const* final { return "assertion failure"; }
};

[[nodiscard]] auto get_fail_action() -> FailAction;
void set_fail_action(FailAction value);

void append_trace(std::string& out, std::stacktrace const& trace);
void print_trace(std::stacktrace const& trace);

void trigger_failure();

void assert_always(bool predicate, std::stacktrace const& trace);
inline void assert_debug(bool const predicate, std::stacktrace const& trace) {
	if constexpr (debug_v) {
		assert_always(predicate, trace);
	}
}
} // namespace klib::assertion

template <std::convertible_to<bool> ExprT>
void klib_assert(ExprT const& expression, std::stacktrace const& trace = std::stacktrace::current()) {
	klib::assertion::assert_always(static_cast<bool>(expression), trace);
}

template <std::convertible_to<bool> ExprT>
void klib_assert_debug(ExprT const& expression, std::stacktrace const& trace = std::stacktrace::current()) {
	klib::assertion::assert_debug(static_cast<bool>(expression), trace);
}
