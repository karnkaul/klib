#include <klib/byte_cast.hpp>
#include <numbers>

namespace {
using namespace klib;

static_assert([] {
	constexpr auto forty_two = 42;
	auto const bytes = byte_cast(forty_two);
	return std::bit_cast<int>(bytes) == forty_two;
}());

static_assert([] {
	auto const bytes = byte_cast(std::numbers::pi);
	return std::bit_cast<decltype(std::numbers::pi)>(bytes) == std::numbers::pi;
}());

static_assert([] {
	auto const bytes = byte_cast(true);
	return std::bit_cast<bool>(bytes);
}());
} // namespace
