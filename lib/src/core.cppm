module;

#include "klib/debug.hpp"

#if __has_include(<cxxabi.h>)
#define KLIB_USE_CXA_DEMANGLE
#include <cxxabi.h>
#endif

export module klib.core;

export import std;
export import klib.string;

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
} // namespace klib

export namespace klib {
template <UniquePayloadT Type>
	requires std::equality_comparable<Type>
struct UniqueIdentity {
	constexpr auto operator()(Type const& t) const noexcept -> bool { return t == Type{}; }
};

template <typename... Args>
struct Noop {
	constexpr void operator()(Args&&... /*unused*/) const noexcept {}
};

class Polymorphic {
  public:
	Polymorphic() = default;
	virtual ~Polymorphic() = default;

	Polymorphic(Polymorphic const&) = default;
	Polymorphic(Polymorphic&&) = default;
	auto operator=(Polymorphic const&) -> Polymorphic& = default;
	auto operator=(Polymorphic&&) -> Polymorphic& = default;
};

class MoveOnly {
  public:
	MoveOnly(MoveOnly const&) = delete;
	auto operator=(MoveOnly const&) = delete;

	MoveOnly() = default;
	~MoveOnly() = default;
	MoveOnly(MoveOnly&&) = default;
	auto operator=(MoveOnly&&) -> MoveOnly& = default;
};

class Pinned {
  public:
	Pinned(Pinned const&) = delete;
	Pinned(Pinned&&) = delete;
	auto operator=(Pinned const&) = delete;
	auto operator=(Pinned&&) = delete;

	Pinned() = default;
	~Pinned() = default;
};

template <typename... Ts>
struct Visitor : Ts... {
	using Ts::operator()...;
};

template <typename... Ts>
struct SubVisitor : Ts... {
	using Ts::operator()...;

	template <typename T>
	constexpr void operator()(T&& /*unused*/) const {}
};

template <typename VariantT, typename... Ts>
constexpr auto match(VariantT&& var, Ts&&... funcs) -> decltype(auto) {
	return std::visit(Visitor{std::forward<Ts>(funcs)...}, std::forward<VariantT>(var));
}
} // namespace klib

export namespace klib {
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
} // namespace klib

export namespace klib {
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
} // namespace klib

export namespace klib {
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
} // namespace klib

export namespace klib {
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
} // namespace klib

export namespace klib {
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

export namespace klib {
template <EnumT E>
constexpr auto enable_enum_bitops(E&& /*unused*/) -> bool {
	return false;
}

template <typename E>
concept EnumOpsT = EnumT<E> && enable_enum_bitops(E{});

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

	[[nodiscard]] constexpr auto as_span() const -> std::span<Entry const> { return m_entries; }
	[[nodiscard]] constexpr auto as_span() -> std::span<Entry> { return m_entries; }

  private:
	ContainerT<Entry, Args...> m_entries{};
};

template <EnumT E, template <typename...> typename ContainerT = std::vector, typename... Args>
class EnumNameMap : public EnumMap<E, std::string_view> {
  public:
	using Entry = EnumMap<E, std::string_view>::Entry;

	EnumNameMap() = default;

	explicit constexpr EnumNameMap(std::initializer_list<Entry> const& entries) : EnumMap<E, std::string_view>(entries) {}

	[[nodiscard]] constexpr auto to_name(E const e) const -> std::string_view {
		if (auto const ptr = this->to_value(e)) { return *ptr; }
		return {};
	}

	[[nodiscard]] constexpr auto to_enum(std::string_view const name) const -> std::optional<E> {
		auto const pred = [name](Entry const& entry) { return entry.second == name; };
		auto const entries = this->as_span();
		if (auto const it = std::ranges::find_if(entries, pred); it != entries.end()) { return it->first; }
		return {};
	}
};
} // namespace klib

export namespace klib {
template <std::uint64_t Factor = 1>
struct ByteCount {
	static constexpr std::uint64_t factor_v = Factor;

	constexpr explicit ByteCount(std::int64_t const value = {}) : m_value(value) {}

	template <std::uint64_t OtherFactor>
	constexpr ByteCount(ByteCount<OtherFactor> const bc) : m_value((bc.count() * bc.factor_v) / factor_v) {}

	constexpr auto operator+=(ByteCount const bc) -> ByteCount& {
		m_value += bc.m_value;
		return *this;
	}

	constexpr auto operator-=(ByteCount const bc) -> ByteCount& {
		m_value -= bc.m_value;
		return *this;
	}

	constexpr auto operator*=(ByteCount const bc) -> ByteCount& {
		m_value *= bc.m_value;
		return *this;
	}

	constexpr auto operator/=(std::int64_t const divisor) -> ByteCount& {
		m_value /= divisor;
		return *this;
	}

	[[nodiscard]] constexpr auto count() const -> std::int64_t { return m_value; }

  private:
	std::int64_t m_value{};
};

template <std::uint64_t FactorL, std::uint64_t FactorR>
constexpr auto operator<=>(ByteCount<FactorL> const a, ByteCount<FactorR> const b) -> std::strong_ordering {
	return a.count() * a.factor_v <=> b.count() * b.factor_v;
}

template <std::uint64_t FactorL, std::uint64_t FactorR>
constexpr auto operator==(ByteCount<FactorL> const a, ByteCount<FactorR> const b) -> bool {
	return a.count() * std::int64_t(a.factor_v) == b.count() * std::int64_t(b.factor_v);
}

template <std::uint64_t Factor>
constexpr auto operator+(ByteCount<Factor> const a, ByteCount<Factor> const b) -> ByteCount<Factor> {
	auto ret = a;
	ret += b;
	return ret;
}

template <std::uint64_t Factor>
constexpr auto operator-(ByteCount<Factor> const a, ByteCount<Factor> const b) -> ByteCount<Factor> {
	auto ret = a;
	ret -= b;
	return ret;
}

template <std::uint64_t Factor>
constexpr auto operator*(ByteCount<Factor> const a, ByteCount<Factor> const b) -> ByteCount<Factor> {
	auto ret = a;
	ret *= b;
	return ret;
}

template <std::uint64_t Factor>
constexpr auto operator/(ByteCount<Factor> const a, std::int64_t const divisor) -> ByteCount<Factor> {
	auto ret = a;
	ret /= divisor;
	return ret;
}

template <std::uint64_t Factor>
constexpr auto operator/(ByteCount<Factor> const a, ByteCount<Factor> const b) -> std::int64_t {
	return a.count() / b.count();
}

using Bytes = ByteCount<1>;
using KibiBytes = ByteCount<kibi_v>;
using MebiBytes = ByteCount<mebi_v>;
using GibiBytes = ByteCount<gibi_v>;
using TebiBytes = ByteCount<tebi_v>;
using KiloBytes = ByteCount<kilo_v>;
using MegaBytes = ByteCount<mega_v>;
using GigaBytes = ByteCount<giga_v>;
using TeraBytes = ByteCount<tera_v>;

namespace literals {
constexpr auto operator""_B(unsigned long long value) { return Bytes{std::int64_t(value)}; }
constexpr auto operator""_KiB(unsigned long long value) { return KibiBytes{std::int64_t(value)}; }
constexpr auto operator""_MiB(unsigned long long value) { return MebiBytes{std::int64_t(value)}; }
constexpr auto operator""_GiB(unsigned long long value) { return GibiBytes{std::int64_t(value)}; }
constexpr auto operator""_TiB(unsigned long long value) { return TebiBytes{std::int64_t(value)}; }
constexpr auto operator""_KB(unsigned long long value) { return KiloBytes{std::int64_t(value)}; }
constexpr auto operator""_MB(unsigned long long value) { return MegaBytes{std::int64_t(value)}; }
constexpr auto operator""_GB(unsigned long long value) { return GigaBytes{std::int64_t(value)}; }
constexpr auto operator""_TB(unsigned long long value) { return TeraBytes{std::int64_t(value)}; }
} // namespace literals
} // namespace klib

export namespace klib {
template <typename Type>
	requires(std::floating_point<Type> || std::signed_integral<Type>)
constexpr auto abs(Type const t) -> Type {
	return t < Type(0) ? -t : t;
}
} // namespace klib

export template <klib::EnumOpsT E>
constexpr auto operator|=(E& out, E const b) -> E& {
	out = E{std::underlying_type_t<E>(std::to_underlying(out) | std::to_underlying(b))};
	return out;
}

export template <klib::EnumOpsT E>
constexpr auto operator|(E const a, E const b) -> E {
	auto ret = a;
	ret |= b;
	return ret;
}

export template <klib::EnumOpsT E>
constexpr auto operator&=(E& out, E const b) -> E& {
	out = E{std::underlying_type_t<E>(std::to_underlying(out) & std::to_underlying(b))};
	return out;
}

export template <klib::EnumOpsT E>
constexpr auto operator&(E const a, E const b) -> E {
	auto ret = a;
	ret &= b;
	return ret;
}

export template <klib::EnumOpsT E>
constexpr auto operator~(E const e) -> E {
	return E{std::underlying_type_t<E>(~std::to_underlying(e))};
}

template <>
struct std::formatter<klib::Version> : klib::FormatParser {
	static auto format(klib::Version const& version, format_context& fc) -> format_context::iterator {
		return format_to(fc.out(), "v{}.{}.{}", version.major, version.minor, version.patch);
	}
};
