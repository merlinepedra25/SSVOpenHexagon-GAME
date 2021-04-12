// Copyright (c) 2013-2020 Vittorio Romeo
// License: Academic Free License ("AFL") v. 3.0
// AFL License page: https://opensource.org/licenses/AFL-3.0

#pragma once

#include "SSVOpenHexagon/Global/Assert.hpp"

#include <SFML/Graphics/Color.hpp>

#include <cmath>

namespace hg::Utils {

[[nodiscard, gnu::always_inline, gnu::pure]] inline sf::Color getColorDarkened(
    sf::Color mColor, const float mMultiplier)
{
    SSVOH_ASSERT(mMultiplier != 0.f);

    mColor.r = static_cast<float>(mColor.r) / mMultiplier;
    mColor.b = static_cast<float>(mColor.b) / mMultiplier;
    mColor.g = static_cast<float>(mColor.g) / mMultiplier;

    return mColor;
}

[[nodiscard, gnu::always_inline, gnu::pure]] inline sf::Color transformHue(
    const sf::Color& in, const float H)
{
    const float u{std::cos(H * 3.14f / 180.f)};
    const float w{std::sin(H * 3.14f / 180.f)};

    return sf::Color{
        //
        static_cast<sf::Uint8>((.701 * u + .168 * w) * in.r +
                               (-.587 * u + .330 * w) * in.g +
                               (-.114 * u - .497 * w) * in.b),

        static_cast<sf::Uint8>((-.299 * u - .328 * w) * in.r +
                               (.413 * u + .035 * w) * in.g +
                               (-.114 * u + .292 * w) * in.b),

        static_cast<sf::Uint8>((-.3 * u + 1.25 * w) * in.r +
                               (-.588 * u - 1.05 * w) * in.g +
                               (.886 * u - .203 * w) * in.b),

        static_cast<sf::Uint8>(255) //
    };
}

[[nodiscard, gnu::always_inline, gnu::pure]] inline sf::Color getColorFromHue(
    const float hue) noexcept
{
    SSVOH_ASSERT(hue >= 0.f && hue <= 1.f);

    const int i = std::floor(hue * 6.f);

    const float f = (hue * 6.f) - i;
    const float q = 1.f - f;
    const float t = f;

    const auto ret = [](const float r, const float g, const float b)
    { return sf::Color(r * 255.f, g * 255.f, b * 255.f); };

    switch(i)
    {
        case 0: return ret(1.f, t, 0.f);
        case 1: return ret(q, 1.f, 0.f);
        case 2: return ret(0.f, 1.f, t);
        case 3: return ret(0.f, q, 1.f);
        case 4: return ret(t, 0.f, 1.f);
    }

    return ret(1.f, 0.f, q);
}

[[nodiscard, gnu::always_inline, gnu::pure]] inline sf::Uint8 componentClamp(
    const float value)
{
    if(value > 255.f)
    {
        return sf::Uint8(255);
    }

    if(value < 0.f)
    {
        return sf::Uint8(0);
    }

    return static_cast<sf::Uint8>(value);
}

} // namespace hg::Utils
