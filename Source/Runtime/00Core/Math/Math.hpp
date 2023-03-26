#pragma once
#include <glm/common.hpp>

#include <concepts>

namespace axe::math
{

using namespace glm;

inline constexpr std::integral auto round_up(std::integral auto val, std::integral auto mul) noexcept
{
    return ((val + mul - 1) / mul) * mul;
}

inline constexpr std::integral auto round_down(std::integral auto val, std::integral auto mul) noexcept
{
    return val - val % mul;
}

}  // namespace axe::math