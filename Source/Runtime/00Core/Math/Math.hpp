#pragma once
#include <glm/common.hpp>
namespace axe::math
{

using namespace glm;

template <typename T>
concept Integral = std::is_integral<T>::value;

inline constexpr Integral auto round_up(Integral auto val, Integral auto mul) noexcept
{
    return ((val + mul - 1) / mul) * mul;
}

inline constexpr Integral auto round_down(Integral auto val, Integral auto mul) noexcept
{
    return val - val % mul;
}

}  // namespace axe::math