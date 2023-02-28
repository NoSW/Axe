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

#ifdef _MSC_VER
#pragma warning(disable : 4067)  // warning C4067: unexpected tokens following preprocessor directive - expected a newline
#endif

namespace axe::reflection
{
// clang-format off
#if AXE_(false, "Deprecated registration method of enum reflection")

template <typename E, std::enable_if_t<std::is_enum<E>::value, bool> = true>
constexpr auto operator*(E e1, E e2) { return std::array<E, 0>{}; }

template <typename E, std::enable_if_t<std::is_enum<E>::value, bool> = true>
constexpr std::string_view operator-(E e) { return ""; }

template <typename E, std::enable_if_t<std::is_enum<E>::value, bool> = true>
constexpr static auto enum_macro_impl()
{
     constexpr auto s = ((E)0) * ((E)0);
     auto longValues  = *((E)0);
     std::array<std::pair<E, std::string_view>, longValues.size()> entries;
     std::size_t begin = 0, cnt = 0;

     const auto pushEntry = [&](const std::string_view entry)
     {
         bool isAssigned = entry.find('=') < entry.size();
         std::size_t bpos = 0;
         while (entry[bpos] == ' ') { bpos++; }
         std::size_t epos = bpos;
         while (epos < entry.size() && entry[epos] != ' ' && entry[epos] != '=') { epos++; }
         entries[cnt].second = entry.substr(bpos, epos - bpos);
         entries[cnt].first  = (E)(isAssigned ? longValues[cnt] : (cnt ? (((long long)entries[cnt - 1].first) + 1) : 0));
         cnt++;
     };

     for (std::size_t i = 0; i < s.size(); ++i) { if (s[i] == ',') { pushEntry(s.substr(begin, i - begin)); begin = i + 1; } }
     if (begin < s.size()) { pushEntry(s.substr(begin, s.size() - begin)); }
     return entries;
 }

template <typename E, std::enable_if_t<std::is_enum<E>::value, bool> = true>
constexpr static auto enum_entries()
{
     constexpr auto s = ((E)0) * ((E)0);
     if constexpr (s.empty()) { return magic_enum::enum_entries<E>(); }
     else { return enum_macro_impl<E>(); }
 }

#define  AXE_REFLECTION_ENUM_INIT(E, ...) __VA_ARGS__ };                                    \
    constexpr std::string_view operator*(E e1, E e2) { return #__VA_ARGS__; }               \
    constexpr auto operator*(E e) { long long __VA_ARGS__; return std::array{__VA_ARGS__};

#else 
template <typename E, std::enable_if_t<std::is_enum<E>::value, bool> = true>
constexpr static auto enum_entries() { return magic_enum::enum_entries<E>(); }
#endif

template <typename E, std::enable_if_t<std::is_enum<E>::value, bool> = true>
constexpr static E enum_value(std::string_view name)
{
     const auto entries = enum_entries<E>();
     for (auto i = 0; i < entries.size(); ++i) { if (entries[i].second == name) { return entries[i].first; } }
     return (E)-1;
 }

template <typename E, std::enable_if_t<std::is_enum<E>::value, bool> = true>
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

template <typename E, std::enable_if_t<std::is_enum<E>::value, bool> = true>
[[nodiscard]] inline string to_string(E value)
{
    return string(axe::reflection::enum_name(value));
}

}  // namespace std
