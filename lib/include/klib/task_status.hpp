#pragma once
#include <klib/task_fwd.hpp>

namespace klib {
enum class TaskStatus : int { None, Queued, Dropped, Executing, Completed };
} // namespace klib
