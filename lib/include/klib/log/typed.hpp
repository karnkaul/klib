#pragma once
#include "klib/demangle.hpp"
#include "klib/log/tagged.hpp"

namespace klib::log {
template <typename Type>
class Typed : public Tagged {
  public:
	explicit Typed(Level max_level = Level::Debug) : Tagged(demangled_name<Type>(), max_level) {}
};
} // namespace klib::log
