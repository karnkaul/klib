module;

#include "klib/debug.hpp"

export module klib.task;

export import std;

import klib.core;

export namespace klib::task {
enum struct ThreadCount : std::uint8_t { Minimum = 1 };
enum struct ElementCount : std::size_t { Unbounded = 0 }; // NOLINT(performance-enum-size)

[[nodiscard]] auto get_max_threads() -> ThreadCount { return ThreadCount(std::thread::hardware_concurrency()); }

struct QueueCreateInfo {
	ThreadCount thread_count{get_max_threads()};
	ElementCount max_elements{ElementCount::Unbounded};
};

enum struct Id : std::uint64_t { None = 0 }; // NOLINT(performance-enum-size)

enum class Status : std::int8_t { None, Queued, Dropped, Executing, Completed };

class Task {
  public:
	using Status = task::Status;
	using Id = task::Id;

	virtual ~Task() = default;

	Task() = default;
	Task(Task const&) = delete;
	Task(Task&&) = delete;
	auto operator=(Task const&) -> Task& = delete;
	auto operator=(Task&&) -> Task& = delete;

	[[nodiscard]] auto get_id() const -> Id { return m_id; }
	[[nodiscard]] auto get_status() const -> Status { return m_status; }
	[[nodiscard]] auto is_busy() const -> bool { return m_busy; }

	void wait() { m_busy.wait(true); }

  protected:
	virtual void execute() = 0;

  private:
	void do_execute();
	void do_drop();

	void finalize();

	std::atomic<Status> m_status{};
	std::atomic<bool> m_busy{};
	Id m_id{Id::None};

	friend class Queue;
};

class Queue {
  public:
	using CreateInfo = QueueCreateInfo;

	explicit Queue(CreateInfo create_info = {});

	[[nodiscard]] auto thread_count() const -> ThreadCount;
	[[nodiscard]] auto max_elements() const -> ElementCount;
	[[nodiscard]] auto enqueued_count() const -> std::size_t;
	[[nodiscard]] auto is_empty() const -> bool { return enqueued_count() == 0; }
	[[nodiscard]] auto can_enqueue(std::size_t count = 1) const -> bool;

	auto enqueue(Task& task) -> bool;
	auto enqueue(std::span<Task* const> tasks) -> bool;
	auto fork_join(std::span<Task* const> tasks) -> Task::Status;

	void pause();
	void resume();
	void drain_and_wait();
	void drop_enqueued();

  private:
	struct Impl;
	struct Deleter {
		void operator()(Impl* ptr) const noexcept;
	};
	std::unique_ptr<Impl, Deleter> m_impl{};
};
} // namespace klib::task

namespace klib::task {
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

struct Queue::Impl {
	Impl(Impl const&) = delete;
	Impl(Impl&&) = delete;
	auto operator=(Impl const&) = delete;
	auto operator=(Impl&&) = delete;

	explicit Impl(CreateInfo const& create_info) : m_create_info(create_info) { create_workers(); }

	~Impl() {
		drop_enqueued();
		destroy_workers();
	}

	[[nodiscard]] auto thread_count() const -> ThreadCount { return m_create_info.thread_count; }

	[[nodiscard]] auto enqueued_count() const -> std::size_t {
		auto lock = std::scoped_lock{m_mutex};
		return m_queue.size();
	}

	[[nodiscard]] auto can_enqueue(std::size_t const count) const -> bool {
		if (m_draining) { return false; }
		if (m_create_info.max_elements == ElementCount::Unbounded) { return true; }
		return enqueued_count() + count < std::size_t(m_create_info.max_elements);
	}

	auto enqueue(std::span<Task* const> tasks) -> bool {
		if (tasks.empty()) { return true; }
		if (!can_enqueue(tasks.size())) { return false; }
		for (auto* task : tasks) {
			KLIB_ASSERT(!task->is_busy());
			if (task->m_id == Task::Id::None) { task->m_id = Task::Id{++m_prev_id}; }
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

	auto fork_join(std::span<Task* const> tasks) -> Task::Status {
		if (tasks.empty()) { return Task::Status::None; }
		if (!enqueue(tasks)) { return Task::Status::Dropped; }
		auto const got_dropped = [](Task* task) {
			task->wait();
			return task->m_status == Task::Status::Dropped;
		};
		if (std::ranges::any_of(tasks, got_dropped)) { return Task::Status::Dropped; }
		return Task::Status::Completed;
	}

	void pause() { m_paused = true; }

	void resume() {
		if (!m_paused) { return; }
		m_paused = false;
		m_work_cv.notify_all();
	}

	void drain_and_wait() {
		resume();
		m_draining = true;
		auto lock = std::unique_lock{m_mutex};
		m_empty_cv.wait(lock, [this] { return m_queue.empty(); });
		KLIB_ASSERT(m_queue.empty());
		lock.unlock();
		recreate_workers();
		m_draining = false;
	}

	void drop_enqueued() {
		auto lock = std::scoped_lock{m_mutex};
		for (auto* task : m_queue) { task->do_drop(); }
		m_queue.clear();
	}

  private:
	void create_workers() {
		auto const count = std::size_t(m_create_info.thread_count);
		m_threads.reserve(count);
		while (m_threads.size() < count) {
			m_threads.emplace_back([this](std::stop_token const& s) { thunk(s); });
		}
	}

	void destroy_workers() {
		for (auto& thread : m_threads) { thread.request_stop(); }
		m_work_cv.notify_all();
		m_threads.clear();
	}

	void recreate_workers() {
		destroy_workers();
		create_workers();
	}

	void thunk(std::stop_token const& s) {
		while (!s.stop_requested()) {
			auto lock = std::unique_lock{m_mutex};
			if (!m_work_cv.wait(lock, s, [this] { return !m_paused && !m_queue.empty(); })) { return; }
			if (s.stop_requested()) { return; }
			auto* task = m_queue.front();
			m_queue.pop_front();
			auto const observed_empty = m_queue.empty();
			lock.unlock();
			task->do_execute();
			if (observed_empty) { m_empty_cv.notify_one(); }
		}
	}

	CreateInfo m_create_info{};
	mutable std::mutex m_mutex{};
	std::condition_variable_any m_work_cv{};
	std::condition_variable m_empty_cv{};
	std::atomic_bool m_paused{};
	std::atomic_bool m_draining{};

	std::deque<Task*> m_queue{};
	std::vector<std::jthread> m_threads{};

	std::underlying_type_t<Task::Id> m_prev_id{};
};

void Queue::Deleter::operator()(Impl* ptr) const noexcept { std::default_delete<Impl>{}(ptr); }

Queue::Queue(CreateInfo create_info) {
	create_info.thread_count = std::clamp(create_info.thread_count, ThreadCount::Minimum, get_max_threads());
	m_impl.reset(new Impl(create_info)); // NOLINT(cppcoreguidelines-owning-memory)
}

auto Queue::thread_count() const -> ThreadCount {
	if (!m_impl) { return ThreadCount{0}; }
	return m_impl->thread_count();
}

auto Queue::enqueued_count() const -> std::size_t {
	if (!m_impl) { return 0; }
	return m_impl->enqueued_count();
}

auto Queue::can_enqueue(std::size_t const count) const -> bool {
	if (!m_impl) { return false; }
	return m_impl->can_enqueue(count);
}

auto Queue::enqueue(Task& task) -> bool {
	auto const tasks = std::array{&task};
	return enqueue(tasks);
}

auto Queue::enqueue(std::span<Task* const> tasks) -> bool {
	if (!m_impl) { return false; }
	return m_impl->enqueue(tasks);
}

auto Queue::fork_join(std::span<Task* const> tasks) -> Task::Status {
	if (!m_impl) { return Task::Status::None; }
	return m_impl->fork_join(tasks);
}

void Queue::pause() {
	if (!m_impl) { return; }
	m_impl->pause();
}

void Queue::resume() {
	if (!m_impl) { return; }
	m_impl->resume();
}

void Queue::drain_and_wait() {
	if (!m_impl) { return; }
	m_impl->drain_and_wait();
}

void Queue::drop_enqueued() {
	if (!m_impl) { return; }
	m_impl->drop_enqueued();
}
} // namespace klib::task
