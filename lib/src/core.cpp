#include "klib/core/assert.hpp"
#include "klib/core/env.hpp"
#include "klib/core/file_io.hpp"
#include "klib/core/string/from_chars.hpp"
#include "klib/core/version.hpp"
#include "klib/core/vigenere_cipher.hpp"
#include <algorithm>
#include <array>
#include <atomic>
#include <filesystem>
#include <fstream>
#include <functional>
#include <print>

#ifdef __linux__
#include <linux/limits.h>
#include <unistd.h>
#endif

#ifdef _WIN32
#include <Windows.h>
#endif

#pragma region version

auto std::formatter<klib::Version>::format(klib::Version const& version, format_context& fc)
	-> format_context::iterator {
	return format_to(fc.out(), "v{}.{}.{}", version.major, version.minor, version.patch);
}

auto klib::to_version(std::string_view text) -> Version {
	if (text.starts_with('v')) {
		text.remove_prefix(1);
	}
	auto fc = FromChars{.text = text};
	auto ret = Version{};
	if (!fc(ret.major) || !fc.advance_if('.') || !fc(ret.minor) || !fc.advance_if('.') || !fc(ret.patch)) {
		return {};
	}
	return ret;
}

#pragma endregion

#pragma region assert

namespace klib {
namespace assertion {
namespace {
// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
auto g_fail_action = std::atomic<assertion::FailAction>{assertion::FailAction::Throw};

auto is_internal(std::string_view const trace_description) -> bool {
	static constexpr auto phrases_v = std::array{
		"!invoke_main+",
		"start_call_main",
		"register_frame_ctor",
	};
	if (trace_description.empty()) {
		return true;
	}
	return std::ranges::any_of(phrases_v, [trace_description](std::string_view const phrase) {
		return trace_description.contains(phrase);
	});
}
} // namespace
} // namespace assertion

auto assertion::get_fail_action() -> FailAction { return g_fail_action; }

void assertion::set_fail_action(FailAction const value) { g_fail_action = value; }

void assertion::append_trace(std::string& out, std::stacktrace const& trace) {
	for (auto const& entry : trace) {
		auto const description = entry.description();
		if (is_internal(description)) {
			return;
		}
		std::format_to(std::back_inserter(out), "  {}", description);
		if constexpr (debug_v) {
			std::string const file = entry.source_file();
			if (!file.empty() && entry.source_line() > 0) {
				std::format_to(std::back_inserter(out), " [{}:{}]", file, entry.source_line());
			}
		}
		out += '\n';
	}
}

void assertion::print_trace(std::stacktrace const& trace) {
	auto msg = std::string{"assertion failed\n"};
	append_trace(msg, trace);
	std::println(stderr, "{}", msg);
}

void assertion::trigger_failure() {
	switch (g_fail_action) {
	case FailAction::Throw: throw Failure{};
	case FailAction::Terminate: std::terminate(); return;
	default: return;
	}
}

void assertion::assert_always(bool const predicate, std::stacktrace const& trace) {
	if (predicate) {
		return;
	}

	print_trace(trace);
	trigger_failure();
}
} // namespace klib

#pragma endregion

#pragma region vigenere

namespace klib {
namespace {
class VigenereCipher {
  public:
	explicit constexpr VigenereCipher(std::string_view const key) : m_key(key) { klib_assert(!m_key.empty()); }

	constexpr void encrypt(std::string_view const src, std::span<char> dst) const { process(src, dst, &shift); }
	constexpr void decrypt(std::string_view const src, std::span<char> dst) const { process(src, dst, &unshift); }

  private:
	static constexpr auto min_shift_v = 32;
	static constexpr auto max_shift_v = 126;
	static constexpr auto shift_mod_v = max_shift_v - min_shift_v + 1;

	[[nodiscard]] static constexpr auto in_range(char const ch) -> bool {
		return int(ch) >= min_shift_v && int(ch) <= max_shift_v;
	}

	[[nodiscard]] static constexpr auto shift(char const ch, int const distance) -> char {
		if (ch < min_shift_v || ch > max_shift_v) {
			return ch;
		}
		auto const ret = int(ch) - min_shift_v + distance;
		klib_assert(ret >= 0);
		return char((ret % shift_mod_v) + min_shift_v);
	}

	[[nodiscard]] static constexpr auto unshift(char const ch, int const distance) -> char {
		if (ch < min_shift_v || ch > max_shift_v) {
			return ch;
		}
		auto const ret = int(ch) + shift_mod_v - min_shift_v - distance;
		klib_assert(ret >= 0);
		return char((ret % shift_mod_v) + min_shift_v);
	}

	template <typename F>
	constexpr void process(std::string_view const src, std::span<char> dst, F func) const {
		klib_assert(src.size() <= dst.size());
		if (src.empty()) {
			return;
		}
		for (std::size_t i = 0; i < src.size(); ++i) {
			// NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)
			auto const ch = src[i];
			if (in_range(ch)) {
				auto const j = i % m_key.size();
				// NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)
				auto const distance = static_cast<int>(m_key[j]) - min_shift_v;
				klib_assert(distance >= 0);
				// NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)
				dst[i] = func(src[i], distance);
			} else {
				// NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)
				dst[i] = src[i];
			}
		}
	}

	std::string_view m_key;
};

template <typename F>
auto apply_vigenere(std::string_view const key, std::string_view const input, F func) -> std::string {
	auto ret = std::string{};
	ret.resize(input.size());
	auto const span = std::span{ret.data(), ret.size()};
	auto cipher = klib::VigenereCipher{key};
	std::invoke(func, &cipher, input, span);
	return ret;
}
} // namespace
} // namespace klib

auto klib::vigenere_encrypt(std::string_view const key, std::string_view const input) -> std::string {
	return apply_vigenere(key, input, &VigenereCipher::encrypt);
}

auto klib::vigenere_decrypt(std::string_view const key, std::string_view const input) -> std::string {
	return apply_vigenere(key, input, &VigenereCipher::decrypt);
}

#pragma endregion

#pragma region env

namespace klib {
auto env::exe_path() -> std::string const& {
	static std::string const ret = [] {
		auto ret = std::string{};
#ifdef _WIN32
		auto buffer = std::array<char, MAX_PATH>{};
		DWORD length = GetModuleFileNameA(nullptr, buffer.data(), buffer.size());
		if (length == 0) {
			return ret;
		}
		ret = std::string{buffer.data(), length};
#elifdef __linux__
		auto buffer = std::array<char, PATH_MAX>{};
		ssize_t length = ::readlink("/proc/self/exe", buffer.data(), buffer.size());
		if (length == -1) {
			return ret;
		}
		ret = std::string{buffer.data(), std::size_t(length)};
#endif
		return ret;
	}();
	return ret;
}

auto env::get_var(CString const key) -> CString {
	if (key.as_view().empty()) {
		return {};
	}
#if defined(_WIN32) && defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
#endif
	// NOLINTNEXTLINE(concurrency-mt-unsafe)
	return std::getenv(key.c_str());
#if defined(_WIN32) && defined(__clang__)
#pragma clang diagnostic pop
#endif
}

} // namespace klib

#pragma endregion

#pragma region file_io

namespace klib {
namespace fs = std::filesystem;

namespace {
template <typename ContainerT>
	requires(sizeof(typename ContainerT::value_type) == 1)
auto read_file_bytes_impl(ContainerT& out, CString const path) -> bool {
	auto file = std::ifstream{path.c_str(), std::ios::binary | std::ios::ate};
	if (!file.is_open()) {
		return false;
	}
	auto const size = file.tellg();
	file.seekg(0, std::ios::beg);
	out.resize(std::size_t(size));
	void* first = out.data();
	file.read(static_cast<char*>(first), size);
	return true;
}
} // namespace

} // namespace klib

auto klib::read_file_bytes_to(std::vector<std::byte>& out, CString const path) -> bool {
	return read_file_bytes_impl(out, path);
}

auto klib::read_file_bytes_to(std::string& out, CString const path) -> bool { return read_file_bytes_impl(out, path); }

auto klib::write_bytes_to_file(std::span<std::byte const> bytes, CString const path) -> bool {
	auto file = std::ofstream{path.c_str(), std::ios::binary};
	if (!file) {
		return false;
	}
	void const* data = bytes.data();
	file.write(static_cast<char const*>(data), std::streamsize(bytes.size()));
	return file.good();
}

auto klib::resolve_symlink(std::string_view const path, int const max_iters) -> std::string {
	auto real_path = fs::path{path};
	auto err = std::error_code{};
	for (auto iter = 0; iter < max_iters; ++iter) {
		if (real_path.empty()) {
			return {};
		}
		if (fs::is_symlink(real_path, err)) {
			real_path = fs::read_symlink(real_path, err);
			if (err != std::errc{}) {
				return {};
			}
			continue;
		}

		return real_path.string();
	}
	return {};
}

#pragma endregion
