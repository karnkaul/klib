#pragma once

namespace klib::task {
enum class Status : int { None, Queued, Dropped, Executing, Completed };
} // namespace klib::task
