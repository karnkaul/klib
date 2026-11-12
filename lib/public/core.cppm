export module klib.core;

export import klib.string;

#pragma region constants

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
#ifdef KLIB_DEBUG
	true;
#else
	false;
#endif
} // namespace klib

#pragma endregion

#pragma region concepts

namespace klib {
template <typename Type>
concept StringyT = std::same_as<Type, std::string> || std::same_as<Type, std::string_view>;

template <typename Type>
concept NotBoolT = !std::same_as<Type, bool>;

template <typename Type>
concept EnumT = std::is_enum_v<Type>;

template <typename Type>
concept MemcpyAble = std::is_trivially_copyable_v<Type>;

template <typename Type>
concept PolymorphicT = std::is_polymorphic_v<Type>;
} // namespace klib

#pragma endregion

#pragma region base_types

export namespace klib {
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
} // namespace klib

#pragma endregion

#pragma region hash_combine

export namespace klib {
/// \brief Combine hash into given seed.
///
/// Source: `boost::hash_combine`\n
// <a href="https://www.boost.org/doc/libs/1_55_0/doc/html/hash/reference.html#boost.hash_combine">
/// https://www.boost.org/doc/libs/1_55_0/doc/html/hash/reference.html#boost.hash_combine</a>
/// \param out_seed mutable reference to an existing seed (hash value).
/// \param hash Hash to combine.
constexpr void hash_combine(std::size_t& out_seed, std::size_t const hash) {
	out_seed ^= hash + 0x9e3779b9 + (out_seed << 6) + (out_seed >> 2);
}

/// \brief Combine hashes of multiple objects into given seed.
/// \param out_seed Mutable reference to existing seed (hash value).
/// \param t Objects whose hashes to combine into out_seed.
template <template <typename> typename Hasher = std::hash, typename... Types>
constexpr void hash_combine(std::size_t& out_seed, Types const&... t) {
	(hash_combine(out_seed, Hasher<Types>{}(t)), ...);
}

/// \brief Make a combined hash of multiple objects.
/// \param t Objects whose hashes to combine.
/// \returns Combined hash.
template <template <typename> typename Hasher = std::hash, typename... Types>
constexpr auto make_combined_hash(Types const&... t) -> std::size_t {
	auto ret = std::size_t{};
	hash_combine<Hasher>(ret, t...);
	return ret;
}
} // namespace klib

#pragma endregion

#pragma region meta

export namespace klib {
template <template <typename, typename> typename Cmp, typename T, typename... Ts>
class TypeSelector {
	using Lhs = T;
	using Rhs = TypeSelector<Cmp, Ts...>::type;

  public:
	using type = Cmp<Lhs, Rhs>::type;
};

template <template <typename, typename> typename Cmp, typename T>
class TypeSelector<Cmp, T> {
  public:
	using type = T;
};

template <typename T, typename U>
struct SmallerType {
	using type = std::conditional_t<(sizeof(T) <= sizeof(U)), T, U>;
};

template <typename T, typename U>
struct LargerType {
	using type = std::conditional_t<(sizeof(T) >= sizeof(U)), T, U>;
};

template <typename T, typename... Ts>
using LargestOf = TypeSelector<LargerType, T, Ts...>::type;

template <typename T, typename... Ts>
using SmallestOf = TypeSelector<SmallerType, T, Ts...>::type;

static_assert(std::same_as<LargestOf<char, int, void*>, void*>);
static_assert(std::same_as<SmallestOf<char, double>, char>);
} // namespace klib

#pragma endregion

#pragma region enum_bitops

export namespace klib {
template <EnumT E>
constexpr auto enable_enum_bitops(E&& /*unused*/) -> bool {
	return false;
}

template <typename E>
concept EnumOpsT = EnumT<E> && enable_enum_bitops(E{});
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

#pragma endregion

#pragma region random

export namespace klib {
template <std::integral Type = int, typename Gen>
[[nodiscard]] auto random_int(Gen& generator, Type const lo, Type const hi) -> Type {
	return std::uniform_int_distribution<Type>{lo, hi}(generator);
}

template <std::integral Type = std::size_t, typename Gen>
[[nodiscard]] auto random_index(Gen& generator, Type const size) -> Type {
	if (size == Type(0)) {
		return Type{};
	}
	return random_int<Type>(generator, 0, size - 1);
}

template <std::floating_point Type = float, typename Gen>
[[nodiscard]] auto random_float(Gen& generator, Type const lo, Type const hi) -> Type {
	return std::uniform_real_distribution<Type>{lo, hi}(generator);
}
} // namespace klib

#pragma endregion

#pragma region Visitor

export namespace klib {
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

#pragma endregion

#pragma region ByteCount

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

#pragma endregion

#pragma region Version

export namespace klib {
struct Version {
	std::int64_t major{};
	std::int64_t minor{};
	std::int64_t patch{};

	auto operator<=>(Version const&) const -> std::strong_ordering = default;
};

[[nodiscard]] auto to_version(std::string_view text) -> Version;
} // namespace klib

template <>
struct std::formatter<klib::Version> : klib::FormatParser {
	static auto format(klib::Version const& version, format_context& fc) -> format_context::iterator {
		return format_to(fc.out(), "v{}.{}.{}", version.major, version.minor, version.patch);
	}
};

#pragma endregion

#pragma region assert

export namespace klib::assertion {
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

export template <std::convertible_to<bool> ExprT>
void klib_assert(ExprT const& expression, std::stacktrace const& trace = std::stacktrace::current()) {
	klib::assertion::assert_always(static_cast<bool>(expression), trace);
}

export template <std::convertible_to<bool> ExprT>
void klib_assert_debug(ExprT const& expression, std::stacktrace const& trace = std::stacktrace::current()) {
	klib::assertion::assert_debug(static_cast<bool>(expression), trace);
}

#pragma endregion

#pragma region vigenere

export namespace klib {
[[nodiscard]] auto vigenere_encrypt(std::string_view key, std::string_view input) -> std::string;
[[nodiscard]] auto vigenere_decrypt(std::string_view key, std::string_view input) -> std::string;
} // namespace klib

#pragma endregion

#pragma region env

export namespace klib::env {
[[nodiscard]] auto exe_path() -> std::string const&;

[[nodiscard]] auto get_var(CString key) -> CString;
} // namespace klib::env

#pragma endregion

#pragma region file_io

export namespace klib {
auto read_file_bytes_to(std::vector<std::byte>& out, CString path) -> bool;
auto read_file_bytes_to(std::string& out, CString path) -> bool;

template <typename ContainerT>
	requires(MemcpyAble<typename ContainerT::value_type>)
auto copy_file_bytes_to(ContainerT& out, CString const path) -> bool {
	static constexpr auto t_size_v = sizeof(typename ContainerT::value_type);
	auto bytes = std::vector<std::byte>{};
	if (!read_file_bytes_to(bytes, path) || bytes.size() % t_size_v != 0) {
		return false;
	}
	out.resize(bytes.size() / t_size_v);
	std::memcpy(out.data(), bytes.data(), bytes.size());
	return true;
}

auto write_bytes_to_file(std::span<std::byte const> bytes, CString path) -> bool;

template <MemcpyAble Type>
auto write_to_file(std::span<Type const> data, CString const path) -> bool {
	return write_bytes_to_file(std::as_bytes(data), path);
}

inline auto write_to_file(std::string_view const text, CString const path) -> bool {
	return write_to_file(std::span{text}, path);
}

[[nodiscard]] auto resolve_symlink(std::string_view path, int max_iters = 100) -> std::string;
} // namespace klib

#pragma endregion

#pragma region Ptr

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

	[[nodiscard]] constexpr auto operator*() const -> Type& { return dereference(); }

	[[nodiscard]] constexpr auto operator->() const -> Type* { return &dereference(); }

	constexpr auto operator==(std::nullptr_t) const -> bool { return m_ptr == nullptr; }

	template <std::derived_from<Type> T>
	constexpr auto operator==(T const* t) const -> bool {
		return m_ptr == t;
	}

	[[nodiscard]] explicit(false) constexpr operator bool() const { return m_ptr != nullptr; }

	template <typename T>
	[[nodiscard]] constexpr auto operator==(Ptr<T> const& rhs) const -> bool {
		return get() == rhs.get();
	}

  private:
	[[nodiscard]] constexpr auto dereference() const -> Type& {
		if (!std::is_constant_evaluated()) {
			klib_assert(m_ptr);
		}
		return *m_ptr;
	}

	Type* m_ptr{};
};
} // namespace klib

#pragma endregion

#pragma region Unique

#ifdef _MSC_VER
#define KLIB_NO_UNIQUE_ADDRESS [[msvc::no_unique_address]]
#else
#define KLIB_NO_UNIQUE_ADDRESS [[no_unique_address]]
#endif

export namespace klib {
template <typename Type>
concept UniquePayloadT = std::is_default_constructible_v<Type> && std::move_constructible<Type>;

template <typename... Args>
struct Noop {
	constexpr void operator()(Args&&... /*unused*/) const noexcept {}
};

template <UniquePayloadT Type>
	requires std::equality_comparable<Type>
struct UniqueIdentity {
	constexpr auto operator()(Type const& t) const noexcept -> bool { return t == Type{}; }
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

	constexpr Unique(Type t, Deleter deleter = Deleter{}, Id id = {})
		: m_t(std::move(t)),
		  m_deleter(std::move(deleter)),
		  m_id(std::move(id)) {}

	constexpr Unique(Unique&& rhs) noexcept
		: m_t(std::exchange(rhs.m_t, Type{})),
		  m_deleter(std::move(rhs.m_deleter)),
		  m_id(std::move(rhs.m_id)) {}

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
		if (is_identity()) {
			return;
		}
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

#pragma endregion

#pragma region ScopedDefer

export namespace klib {
class ScopedDefer {
  public:
	using Func = std::move_only_function<void()>;

	template <std::convertible_to<Func> F = Func>
	explicit(false) ScopedDefer(F func = {}) : m_func(std::move(func)) {}

  private:
	struct Id {
		auto operator()(Func const& f) const noexcept -> bool { return f == nullptr; }
	};
	struct Deleter {
		void operator()(Func func) const noexcept { func(); }
	};
	Unique<Func, Deleter, Id> m_func;
};
} // namespace klib

#pragma endregion

#pragma region EnumMap

export namespace klib {
template <EnumT E, typename ValueT, template <typename...> typename ContainerT = std::vector, typename... Args>
class EnumMap {
  public:
	using Entry = std::pair<E, ValueT>;

	EnumMap() = default;

	explicit constexpr EnumMap(std::initializer_list<Entry> const& entries) : m_entries(entries) {}

	[[nodiscard]] constexpr auto to_value(E const e) const -> Ptr<ValueT const> {
		auto const pred = [e](Entry const& entry) {
			return entry.first == e;
		};
		if (auto const it = std::ranges::find_if(m_entries, pred); it != m_entries.end()) {
			return &it->second;
		}
		return nullptr;
	}

	[[nodiscard]] constexpr auto to_enum(ValueT const& value) const -> std::optional<E>
		requires(std::equality_comparable<ValueT>)
	{
		auto const pred = [&value](Entry const& entry) {
			return entry.second == value;
		};
		if (auto const it = std::ranges::find_if(m_entries, pred); it != m_entries.end()) {
			return it->first;
		}
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
		if (auto const ptr = this->to_value(e)) {
			return *ptr;
		}
		return fallback;
	}
};
} // namespace klib

#pragma endregion
