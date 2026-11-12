#pragma once
#include "klib/core/base_types.hpp"
#include "klib/task/id.hpp"
#include "klib/task/status.hpp"
#include "klib/task/task_fwd.hpp"
#include <atomic>

namespace klib::task {
class Task : public Polymorphic, public Pinned {
  public:
	using Status = task::Status;
	using Id = task::Id;

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
} // namespace klib::task
