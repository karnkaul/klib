#include "klib/ptr.hpp"

namespace {
template <typename T>
using Ptr = klib::Ptr<T>;

struct A {};
struct B : A {};

constexpr auto a = A{};
constexpr auto p_a = Ptr{&a};
static_assert(p_a);
static_assert(p_a == &a);

constexpr auto b = B{};
constexpr Ptr<A const> p_b{&b};
static_assert(p_b);
static_assert(p_b == Ptr{&b});
} // namespace
