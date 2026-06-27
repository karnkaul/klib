module;

#include "klib/debug.hpp"

#if __has_include(<cxxabi.h>)
#define KLIB_USE_CXA_DEMANGLE
#include <cxxabi.h>
#endif

export module klib.core;

export import std;

import klib.string;

#include "klib/concepts.hpp"
#include "klib/macros.hpp"

namespace klib {
namespace {
// https://gcc.gnu.org/pipermail/libstdc++/2025-May/061246.html
[[maybe_unused]] auto glibcxx_is_debugger_present() -> bool {
	using namespace std;
	string_view const prefix = "TracerPid:\t"; // populated since Linux 2.6.0
	ifstream in("/proc/self/status");
	string line;
	while (std::getline(in, line)) {
		if (!line.starts_with(prefix)) { continue; }

		string_view tracer = line;
		tracer.remove_prefix(prefix.size());
		if (tracer.size() == 1 && tracer[0] == '0') [[likely]] {
			return false; // Not being traced.
		}

		in.close();
		string_view cmd;
		string proc_dir = "/proc/" + string(tracer) + '/';
		in.open(proc_dir + "comm"); // since Linux 2.6.33
		if (std::getline(in, line)) [[likely]] {
			cmd = line;
		} else {
			in.close();
			in.open(proc_dir + "cmdline");
			if (std::getline(in, line)) {
				// NOLINTNEXTLINE(readability-redundant-string-cstr)
				cmd = line.c_str(); // Only up to first '\0'
			} else {
				return false;
			}
		}

		static constexpr auto known_debuggers = std::array{"gdb", "gdbserver", "lldb-server"};
		return std::ranges::any_of(known_debuggers, [cmd](char const* dbg) { return cmd.ends_with(dbg); });
	}
	return false;
}
} // namespace

namespace assertion {
export enum class FailAction : std::uint8_t { Throw, Terminate, None };

namespace {
// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
auto g_fail_action = std::atomic<assertion::FailAction>{assertion::FailAction::Throw};

auto is_internal(std::string_view const trace_description) -> bool {
	static constexpr auto phrases_v = std::array{
		"!invoke_main+",
		"start_call_main",
		"register_frame_ctor",
	};
	if (trace_description.empty()) { return true; }
	return std::ranges::any_of(phrases_v, [trace_description](std::string_view const phrase) { return trace_description.contains(phrase); });
}
} // namespace
} // namespace assertion
} // namespace klib

export namespace klib {
template <UniquePayloadT Type>
	requires std::equality_comparable<Type>
struct UniqueIdentity {
	constexpr auto operator()(Type const& t) const noexcept -> bool { return t == Type{}; }
};

constexpr std::uint64_t kibi_v = 1024;
constexpr std::uint64_t mebi_v = 1024 * kibi_v;
constexpr std::uint64_t gibi_v = 1024 * mebi_v;
constexpr std::uint64_t tebi_v = 1024 * gibi_v;

constexpr std::uint64_t kilo_v = 1000;
constexpr std::uint64_t mega_v = 1000 * kilo_v;
constexpr std::uint64_t giga_v = 1000 * mega_v;
constexpr std::uint64_t tera_v = 1000 * giga_v;

constexpr auto debug_v =
#if defined(KLIB_DEBUG)
	true;
#else
	false;
#endif

constexpr auto use_stacktrace_v =
#if defined(KLIB_USE_STACKTRACE)
	true;
#else
	false;
#endif

auto is_debugger_attached() -> bool {
#if defined(_WIN32)
	return IsDebuggerPresent() != 0;
#else
	return glibcxx_is_debugger_present();
#endif
}

namespace assertion {
struct Failure : std::exception {
	[[nodiscard]] auto what() const noexcept -> char const* final { return "assertion failure"; }
};

[[nodiscard]] auto get_fail_action() -> FailAction { return g_fail_action; }
void set_fail_action(FailAction const value) { g_fail_action = value; }

void append_trace(std::string& out, std::stacktrace const& trace) {
	if constexpr (use_stacktrace_v) {
		for (auto const& entry : trace) {
			auto const description = entry.description();
			if (is_internal(description)) { return; }
			std::format_to(std::back_inserter(out), "  {}", description);
			if constexpr (debug_v) {
				std::string const file = entry.source_file();
				if (!file.empty() && entry.source_line() > 0) { std::format_to(std::back_inserter(out), " [{}:{}]", file, entry.source_line()); }
			}
			out += '\n';
		}
	}
}

void print(std::string_view expr, std::stacktrace const& trace = KLIB_GET_TRACE()) {
	auto msg = std::format("assertion failed: '{}'\n", expr);
	append_trace(msg, trace);
	std::println(std::cerr, "{}", msg);
}

void trigger_failure() {
	switch (g_fail_action) {
	case FailAction::Throw: throw Failure{};
	case FailAction::Terminate: std::terminate(); return;
	default: return;
	}
}
} // namespace assertion

template <typename... Args>
struct Noop {
	constexpr void operator()(Args&&... /*unused*/) const noexcept {}
};

struct Version {
	std::int64_t major{};
	std::int64_t minor{};
	std::int64_t patch{};

	auto operator<=>(Version const&) const -> std::strong_ordering = default;
};

[[nodiscard]] auto to_version(std::string_view text) -> Version {
	if (text.starts_with('v')) { text.remove_prefix(1); }
	auto fc = FromChars{.text = text};
	auto ret = Version{};
	if (!fc(ret.major) || !fc.advance_if('.') || !fc(ret.minor) || !fc.advance_if('.') || !fc(ret.patch)) { return {}; }
	return ret;
}

/// \brief Wrapper over a non-owning raw pointer.
/// Asserts on attempts to dereference if null.
template <typename Type>
class Ptr {
  public:
	Ptr() = default;

	explicit(false) constexpr Ptr(Type* t) : m_ptr(t) {}

	template <std::convertible_to<Type*> T>
	explicit(false) constexpr Ptr(T t) : m_ptr(static_cast<Type*>(t)) {}

	[[nodiscard]] constexpr auto get() const -> Type* { return m_ptr; }

	template <typename T>
		requires(std::derived_from<Type, T>)
	[[nodiscard]] explicit(false) constexpr operator T*() const {
		return get();
	}

	[[nodiscard]] constexpr auto operator*() const -> Type& {
		KLIB_ASSERT(m_ptr);
		return *m_ptr;
	}

	[[nodiscard]] constexpr auto operator->() const -> Type* {
		KLIB_ASSERT(m_ptr);
		return m_ptr;
	}

	[[nodiscard]] explicit(false) constexpr operator bool() const { return m_ptr != nullptr; }

	template <typename T>
	[[nodiscard]] constexpr auto operator==(Ptr<T> const& rhs) const -> bool {
		return get() == rhs.get();
	}

  private:
	Type* m_ptr{};
};

template <UniquePayloadT Type, typename Deleter = Noop<Type>, typename Id = UniqueIdentity<Type>>
class Unique {
  public:
	using value_type = Type;
	using deleter_type = Deleter;
	using id_type = Id;

	Unique(Unique const&) = delete;
	auto operator=(Unique const&) -> Unique& = delete;

	Unique() = default;

	constexpr Unique(Type t, Deleter deleter = Deleter{}, Id id = {}) : m_t(std::move(t)), m_deleter(std::move(deleter)), m_id(std::move(id)) {}

	constexpr Unique(Unique&& rhs) noexcept : m_t(std::exchange(rhs.m_t, Type{})), m_deleter(std::move(rhs.m_deleter)), m_id(std::move(rhs.m_id)) {}

	constexpr auto operator=(Unique&& rhs) noexcept -> Unique& {
		using std::swap;
		if (this != &rhs) {
			swap(m_t, rhs.m_t);
			swap(m_deleter, rhs.m_deleter);
			swap(m_id, rhs.m_id);
		}
		return *this;
	}

	constexpr ~Unique() {
		if (is_identity()) { return; }
		m_deleter(std::move(m_t));
	}

	[[nodiscard]] constexpr auto is_identity() const -> bool { return m_id(m_t); }

	[[nodiscard]] constexpr auto get() const -> Type const& { return m_t; }
	[[nodiscard]] constexpr auto get() -> Type& { return m_t; }

	[[nodiscard]] constexpr auto get_deleter() const -> deleter_type const& { return m_deleter; }
	[[nodiscard]] constexpr auto get_id() const -> id_type const& { return m_id; }

	constexpr operator Type const&() const { return get(); }
	constexpr operator Type&() const { return get(); }

  private:
	Type m_t;
	KLIB_NO_UNIQUE_ADDRESS Deleter m_deleter;
	KLIB_NO_UNIQUE_ADDRESS Id m_id;
};

[[nodiscard]] auto demangled_name(std::type_info const& info) -> std::string {
#if defined(KLIB_USE_CXA_DEMANGLE)
	static auto cleanup = [](std::string_view in) {
		if (auto const i = in.find('@'); i != std::string_view::npos) { in = in.substr(0, i); }
		return std::string{in};
	};

	auto status = int{};
	auto const buf = std::unique_ptr<char, decltype(std::free)*>{
		abi::__cxa_demangle(info.name(), nullptr, nullptr, &status),
		std::free,
	};
	if (status == 0) { return cleanup(buf.get()); }
#endif
	static constexpr auto prefixes_v = std::array{
		"struct ",
		"class ",
		"enum ",
	};
	auto view = std::string_view{info.name()};
	for (std::string_view const prefix : prefixes_v) {
		if (view.starts_with(prefix)) {
			view.remove_prefix(prefix.size());
			break;
		}
	}
	return std::string{view};
}

#undef KLIB_USE_CXA_DEMANGLE

template <typename Type>
[[nodiscard]] auto demangled_name() -> std::string const& {
	static std::string const ret = [] { return demangled_name(typeid(Type)); }();
	return ret;
}

template <PolymorphicT Type>
[[nodiscard]] auto demangled_name(Type const& t) -> std::string {
	return demangled_name(typeid(t));
}
} // namespace klib

template <>
struct std::formatter<klib::Version> : klib::FormatParser {
	static auto format(klib::Version const& version, format_context& fc) -> format_context::iterator {
		return format_to(fc.out(), "v{}.{}.{}", version.major, version.minor, version.patch);
	}
};
