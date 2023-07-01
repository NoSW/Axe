#pragma once

#include "00Core/Config.hpp"

#include <glm/common.hpp>

#include <concepts>

namespace axe::math
{

using namespace glm;

// return a multiple of mul, which is >= val and closest to val. e.g., round_up(15, 8) => 16
inline constexpr std::unsigned_integral auto round_up(std::unsigned_integral auto val, std::unsigned_integral auto mul) noexcept
{
    return mul == 0 ? val : (((val + mul - 1) / mul) * mul);
}

// return a multiple of mul, which is <= val and closest to val. e.g., round_up(15, 8) => 16
inline constexpr std::unsigned_integral auto round_down(std::unsigned_integral auto val, std::unsigned_integral auto mul) noexcept
{
    return mul == 0 ? val : (val - val % mul);
}

}  // namespace axe::math