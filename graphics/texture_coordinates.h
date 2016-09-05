/*
 * Copyright (C) 2012-2016 Jacob R. Lifshay
 * This file is part of Voxels.
 *
 * Voxels is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Voxels is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with Voxels; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA.
 *
 */

#ifndef GRAPHICS_TEXTURE_COORDINATES_H_
#define GRAPHICS_TEXTURE_COORDINATES_H_

#include <cstdint>
#include <limits>
#include <type_traits>
#include "../util/interpolate.h"

namespace programmerjake
{
namespace voxels
{
namespace graphics
{
struct TextureCoordinates final
{
    typedef float ValueType;
    ValueType u;
    ValueType v;
    static constexpr ValueType valueMin()
    {
        return 0;
    }
    static constexpr ValueType valueMax()
    {
        return std::is_floating_point<ValueType>::value ? 1 : std::numeric_limits<ValueType>::max();
    }
    constexpr TextureCoordinates() : u(), v()
    {
    }
    constexpr TextureCoordinates(ValueType u, ValueType v) : u(u), v(v)
    {
    }
    constexpr static TextureCoordinates maxUMaxV()
    {
        return TextureCoordinates(valueMax(), valueMax());
    }
    constexpr static TextureCoordinates minUMaxV()
    {
        return TextureCoordinates(valueMin(), valueMax());
    }
    constexpr static TextureCoordinates maxUMinV()
    {
        return TextureCoordinates(valueMax(), valueMin());
    }
    constexpr static TextureCoordinates minUMinV()
    {
        return TextureCoordinates(valueMin(), valueMin());
    }

private:
    template <typename T = ValueType, typename U>
    constexpr static typename std::enable_if<std::is_floating_point<T>::value, T>::type roundResult(
        U v)
    {
        return v;
    }
    template <typename T = ValueType, typename U>
    constexpr static typename std::enable_if<!std::is_floating_point<T>::value, T>::type
        roundResult(U v, int = 0)
    {
        return v > 0 ? v + 0.5f : v - 0.5f;
    }

public:
    friend constexpr TextureCoordinates interpolate(float v,
                                                    TextureCoordinates a,
                                                    TextureCoordinates b)
    {
        using util::interpolate;
        return TextureCoordinates(roundResult(interpolate(v, a.u, b.u)),
                                  roundResult(interpolate(v, a.v, b.v)));
    }
    constexpr static TextureCoordinates fromFloat(float u, float v)
    {
        return TextureCoordinates(roundResult(u * valueMax()), roundResult(v * valueMax()));
    }
    constexpr float getFloatU() const
    {
        return u / static_cast<float>(valueMax());
    }
    constexpr float getFloatV() const
    {
        return v / static_cast<float>(valueMax());
    }
};
}
}
}

#endif /* GRAPHICS_TEXTURE_COORDINATES_H_ */
