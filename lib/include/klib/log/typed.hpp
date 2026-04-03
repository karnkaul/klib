#pragma once
#include "klib/demangle.hpp"
#include "klib/log/tagged.hpp"

namespace klib::log {
template <typename Type>
class Typed : public Tagged {
  public:
	explicit Typed() : Tagged(demangled_name<Type>()) {}
};
} // namespace klib::log
