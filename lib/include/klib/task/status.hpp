#pragma once

#include <cstdint>
namespace klib::task {
enum class Status : std::int8_t { None, Queued, Dropped, Executing, Completed };
} // namespace klib::task
