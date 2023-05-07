#pragma once

#include <string>

namespace axe::con
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

}  // namespace axe::con