#pragma once

namespace klib {
template <typename Type>
concept StringyT = std::same_as<Type, std::string> || std::same_as<Type, std::string_view>;

template <typename Type>
concept NumberT = !std::same_as<bool, Type> && (std::integral<Type> || std::floating_point<Type>);

template <typename Type>
concept NotBoolT = !std::same_as<Type, bool>;

template <typename Type>
concept EnumT = std::is_enum_v<Type>;

template <typename Type>
concept MemcpyAble = std::is_trivially_copyable_v<Type>;

template <typename Type>
concept PolymorphicT = std::is_polymorphic_v<Type>;

template <typename Type>
concept UniquePayloadT = std::is_default_constructible_v<Type> && std::move_constructible<Type>;
} // namespace klib
