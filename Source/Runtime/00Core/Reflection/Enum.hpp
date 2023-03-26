/*
   Wrapper of https://github.com/Neargye/magic_enum, a library uses a compiler-specific hack (based on __PRETTY_FUNCTION__ / __FUNCSIG__).

   1. Can't reflect the enum with a forward declaration.
   2. For alias, only the one that occur firstly works. e.g., enum FLAG {FLAG_A = 100, FLAG_B = 100,}, only FLAG_A works
   3. Can't reflect enum exceeding the default range of [-256, 256] (Due to the limited computation of compiler).

   To work with case 2, 3, please instantiate enum_entries(){...} in namespace axe::reflection

*/
#pragma once
#define MAGIC_ENUM_RANGE_MIN -256
#define MAGIC_ENUM_RANGE_MAX 256
#include "magic_enum/magic_enum.hpp"
#if !MAGIC_ENUM_SUPPORTED
#error "Too old compiler that cannot support magic enum";
#endif

#include <concepts>

#ifdef _MSC_VER
#pragma warning(disable : 4067)  // warning C4067: unexpected tokens following preprocessor directive - expected a newline
#endif

template <typename E>
concept T_Enum = std::is_enum_v<E>;

namespace axe::reflection
{
// clang-format off
template <T_Enum E>
constexpr static auto enum_entries() { return magic_enum::enum_entries<E>(); }

template <T_Enum E>
constexpr static E enum_value(std::string_view name)
{
     const auto entries = enum_entries<E>();
     for (auto i = 0; i < entries.size(); ++i) { if (entries[i].second == name) { return entries[i].first; } }
     return (E)-1;
 }

template <T_Enum E>
constexpr static std::string_view enum_name(E value)
{
     const auto entries = enum_entries<E>();
     for (auto i = 0; i < entries.size(); ++i) { if (entries[i].first == value) { return entries[i].second; } }
     return typeid(E).name();
 }

// clang-format on
}  // namespace axe::reflection

using namespace magic_enum::bitwise_operators;

namespace std
{

template <T_Enum E>
[[nodiscard]] inline string to_string(E value)
{
    return string(axe::reflection::enum_name(value));
}

}  // namespace std
