export module klib.task;

export import klib.core;

export namespace klib::task {
enum struct Id : std::uint64_t { None = 0 }; // NOLINT(performance-enum-size)
enum class Status : std::int8_t { None, Queued, Dropped, Executing, Completed };

enum struct ThreadCount : std::uint8_t { Minimum = 1 };
enum struct ElementCount : std::size_t { Unbounded = 0 }; // NOLINT(performance-enum-size)

[[nodiscard]] auto get_max_threads() -> ThreadCount;

struct QueueCreateInfo {
	ThreadCount thread_count{get_max_threads()};
	ElementCount max_elements{ElementCount::Unbounded};
};

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

class Queue {
  public:
	using CreateInfo = QueueCreateInfo;

	Queue(Queue const&) = delete;
	Queue(Queue&&) = delete;
	auto operator=(Queue const&) = delete;
	auto operator=(Queue&&) = delete;

	explicit Queue(CreateInfo const& create_info);
	~Queue();

	[[nodiscard]] auto thread_count() const -> ThreadCount { return m_create_info.thread_count; }

	[[nodiscard]] auto enqueued_count() const -> std::size_t;

	[[nodiscard]] auto is_empty() const -> bool { return enqueued_count() == 0; }

	[[nodiscard]] auto can_enqueue(std::size_t count) const -> bool;

	auto enqueue(Task& task) -> bool;

	auto enqueue(std::span<Ptr<Task> const> tasks) -> bool;

	auto fork_join(std::span<Ptr<Task> const> tasks) -> Task::Status;

	void pause() { m_paused = true; }

	void resume();

	void drain_and_wait();

	void drop_enqueued();

  private:
	void create_workers();
	void destroy_workers();
	void recreate_workers();

	void thunk(std::stop_token const& s);

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
} // namespace klib::task
