#pragma once
#include <array>
#include <concepts>
#include <cstddef>
#include <utility>

namespace klib {
template <std::size_t MaxSize = sizeof(void*)>
class FixedAny {
  public:
	template <typename Type>
		requires(sizeof(Type) <= MaxSize)
	/*implicit*/ FixedAny(Type t) : m_vtable(&VTable::template get<Type>()) {
		m_vtable->move_construct(&t, m_storage.data());
	}

	FixedAny(FixedAny&& rhs) noexcept : m_vtable(rhs.m_vtable) { m_vtable->move_construct(rhs.m_storage.data(), m_storage.data()); }

	auto operator=(FixedAny&& rhs) noexcept -> FixedAny& {
		if (&rhs != this) {
			destroy();
			m_vtable = rhs.m_vtable;
			m_vtable->move_construct(rhs.m_storage.data(), m_storage.data());
		}
		return *this;
	}

	FixedAny(FixedAny const& rhs) : m_vtable(rhs.m_vtable) { m_vtable->copy_construct(rhs.m_storage.data(), m_storage.data()); }

	auto operator=(FixedAny const& rhs) noexcept -> FixedAny& {
		if (&rhs != this) {
			destroy();
			m_vtable = rhs.m_vtable;
			m_vtable->copy_construct(rhs.m_storage.data(), m_storage.data());
		}
		return *this;
	}

	~FixedAny() { destroy(); }

	template <typename Type>
	[[nodiscard]] auto contains() const -> bool {
		return m_vtable == &VTable::template get<Type>();
	}

	template <typename Type>
	[[nodiscard]] auto get_if() const -> Type const* {
		if (!contains<Type>()) { return {}; }
		// NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
		return std::launder(reinterpret_cast<Type const*>(m_storage.data()));
	}

	template <typename Type>
	[[nodiscard]] auto get_if() -> Type* {
		return const_cast<Type*>(std::as_const(*this).template get_if<Type>());
	}

  private:
	struct VTable {
		void (*move_construct)(void* src, void* dst){};
		void (*copy_construct)(void const* src, void* dst){};
		void (*destroy)(void* dst){};

		template <typename Type>
		static auto get() -> VTable const& {
			static auto ret = VTable{
				.move_construct = [](void* src, void* dst) { new (dst) Type(std::move(*static_cast<Type>(src))); },
				.copy_construct = [](void const* src, void* dst) { new (dst) Type(*static_cast<Type>(src)); },
				.destroy = [](void* dst) { std::destroy_at(static_cast<Type*>(dst)); },
			};
			return ret;
		}
	};

	void destroy() {
		if (m_vtable == nullptr) { return; }
		m_vtable->destroy(m_storage.data());
		m_vtable = {};
	}

	alignas(std::max_align_t) std::array<std::byte, MaxSize> m_storage{};
	VTable const* m_vtable{};
};
} // namespace klib
