#pragma once
#include <klib/polymorphic.hpp>
#include <string_view>

namespace klib::args {
class IPrinter : public Polymorphic {
  public:
	virtual void print(std::string_view text) = 0;
	virtual void printerr(std::string_view text) = 0;
};
} // namespace klib::args
