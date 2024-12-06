#pragma once

namespace klib {
class Polymorphic {
  public:
	Polymorphic() = default;
	virtual ~Polymorphic() = default;

	Polymorphic(Polymorphic const&) = default;
	Polymorphic(Polymorphic&&) = default;
	auto operator=(Polymorphic const&) -> Polymorphic& = default;
	auto operator=(Polymorphic&&) -> Polymorphic& = default;
};
} // namespace klib
