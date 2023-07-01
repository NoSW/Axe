#pragma once

#include <string>

namespace axe::str
{

//  utils string functions
template <typename charT>
struct equal_case_insensitive
{
    equal_case_insensitive(const std::locale& loc) : loc_(loc) {}
    bool operator()(charT ch1, charT ch2)
    {
        return std::toupper(ch1, loc_) == std::toupper(ch2, loc_);
    }

private:
    const std::locale& loc_;
};

template <typename T>
std::size_t find_substr_case_insensitive(const T& mainStr, const T& subStr, const std::locale& loc = std::locale())
{
    typename T::const_iterator it = std::search(mainStr.begin(), mainStr.end(),
                                                subStr.begin(), subStr.end(), equal_case_insensitive<typename T::value_type>(loc));
    if (it != mainStr.end()) { return it - mainStr.begin(); }
    else { return mainStr.size(); }
}

// return string_view removing last extension, e.g., "console.log.txt" return "console.log"
constexpr std::string_view rm_ext(const std::string_view filename) noexcept
{
    return filename.substr(0, filename.find_last_of('.'));
}

constexpr std::string_view last_ext(const std::string_view filename) noexcept
{
    return filename.substr(filename.find_last_of('.'));
}

}  // namespace axe::str