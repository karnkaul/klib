#pragma once
#include <klib/task/queue_create_info.hpp>
#include <klib/task/queue_fwd.hpp>
#include <klib/task/task.hpp>
#include <memory>
#include <span>

namespace klib::task {
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
