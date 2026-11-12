module klib.task;

namespace klib {
namespace task {
void Task::do_execute() {
	m_status = Status::Executing;
	try {
		execute();
	} catch (...) {}
	finalize();
}

void Task::do_drop() {
	m_status = Task::Status::Dropped;
	finalize();
}

void Task::finalize() {
	switch (m_status) {
	case Status::Executing: m_status = Status::Completed; break;
	default: break;
	}
	m_busy = false;
	m_busy.notify_all();
}

Queue::Queue(CreateInfo const& create_info) : m_create_info(create_info) {
	m_create_info.thread_count = std::clamp(create_info.thread_count, ThreadCount::Minimum, get_max_threads());
	create_workers();
}

Queue::~Queue() {
	drop_enqueued();
	destroy_workers();
}

auto Queue::enqueued_count() const -> std::size_t {
	auto lock = std::scoped_lock{m_mutex};
	return m_queue.size();
}

auto Queue::can_enqueue(std::size_t const count) const -> bool {
	if (m_draining) {
		return false;
	}
	if (m_create_info.max_elements == ElementCount::Unbounded) {
		return true;
	}
	return enqueued_count() + count < std::size_t(m_create_info.max_elements);
}

auto Queue::enqueue(Task& task) -> bool {
	auto const tasks = std::array{Ptr{&task}};
	return enqueue(tasks);
}

auto Queue::enqueue(std::span<Ptr<Task> const> tasks) -> bool {
	if (tasks.empty()) {
		return true;
	}
	if (!can_enqueue(tasks.size())) {
		return false;
	}
	for (auto const task : tasks) {
		klib_assert(!task->is_busy());
		if (task->m_id == Task::Id::None) {
			task->m_id = Task::Id{++m_prev_id};
		}
		task->m_status = Task::Status::Queued;
		task->m_busy = true;
	}
	auto lock = std::unique_lock{m_mutex};
	m_queue.insert(m_queue.end(), tasks.begin(), tasks.end());
	lock.unlock();
	if (tasks.size() > 1) {
		m_work_cv.notify_all();
	} else {
		m_work_cv.notify_one();
	}
	return true;
}

auto Queue::fork_join(std::span<Ptr<Task> const> tasks) -> Task::Status {
	if (tasks.empty()) {
		return Task::Status::None;
	}
	if (!enqueue(tasks)) {
		return Task::Status::Dropped;
	}
	auto const got_dropped = [](Task* task) {
		task->wait();
		return task->m_status == Task::Status::Dropped;
	};
	if (std::ranges::any_of(tasks, got_dropped)) {
		return Task::Status::Dropped;
	}
	return Task::Status::Completed;
}

void Queue::resume() {
	if (!m_paused) {
		return;
	}
	m_paused = false;
	m_work_cv.notify_all();
}

void Queue::drain_and_wait() {
	resume();
	m_draining = true;
	auto lock = std::unique_lock{m_mutex};
	m_empty_cv.wait(lock, [this] {
		return m_queue.empty();
	});
	klib_assert(m_queue.empty());
	lock.unlock();
	recreate_workers();
	m_draining = false;
}

void Queue::drop_enqueued() {
	auto lock = std::scoped_lock{m_mutex};
	for (auto* task : m_queue) {
		task->do_drop();
	}
	m_queue.clear();
}

void Queue::create_workers() {
	auto const count = std::size_t(m_create_info.thread_count);
	m_threads.reserve(count);
	while (m_threads.size() < count) {
		m_threads.emplace_back([this](std::stop_token const& s) {
			thunk(s);
		});
	}
}

void Queue::destroy_workers() {
	for (auto& thread : m_threads) {
		thread.request_stop();
	}
	m_work_cv.notify_all();
	m_threads.clear();
}

void Queue::recreate_workers() {
	destroy_workers();
	create_workers();
}

void Queue::thunk(std::stop_token const& s) {
	while (!s.stop_requested()) {
		auto lock = std::unique_lock{m_mutex};
		if (!m_work_cv.wait(lock, s, [this] {
				return !m_paused && !m_queue.empty();
			})) {
			return;
		}
		if (s.stop_requested()) {
			return;
		}
		auto* task = m_queue.front();
		m_queue.pop_front();
		auto const observed_empty = m_queue.empty();
		lock.unlock();
		task->do_execute();
		if (observed_empty) {
			m_empty_cv.notify_one();
		}
	}
}
} // namespace task

auto task::get_max_threads() -> ThreadCount { return ThreadCount(std::thread::hardware_concurrency()); }
} // namespace klib
