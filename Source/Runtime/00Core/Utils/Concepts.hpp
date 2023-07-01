#pragma once
#include <concepts>

namespace axe
{

// plain old data
template <typename T>
concept T_POD = std::is_trivially_copyable_v<T> &&
                std::is_standard_layout_v<T> &&
                std::is_pod_v<T>;

template <typename E>
concept T_enum = std::is_enum_v<E>;

template <typename E>
concept T_enum_or_integral = std::is_enum_v<E> || std::is_integral_v<E>;

template <typename T>
concept T_boolable = std::convertible_to<T, bool>;

}  // namespace axe