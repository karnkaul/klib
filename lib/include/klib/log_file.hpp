#pragma once
#include <klib/log.hpp>
#include <memory>

namespace klib::log {
class FileSink : public Sink {
  public:
	explicit FileSink(CString path = "debug.log");

	void on_log(Input const& input, CString text) final;

  private:
	struct Impl;
	struct Deleter {
		void operator()(Impl* ptr) const noexcept;
	};
	std::unique_ptr<Impl, Deleter> m_impl;
};
} // namespace klib::log
