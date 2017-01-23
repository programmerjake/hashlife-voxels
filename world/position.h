/*
 * Copyright (C) 2012-2017 Jacob R. Lifshay
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

#ifndef WORLD_POSITION_H_
#define WORLD_POSITION_H_

#include "../util/vector.h"
#include "dimension.h"
#include <cstdint>

namespace programmerjake
{
namespace voxels
{
namespace world
{
template <typename T>
struct Position3 : public util::Vector3<T>
{
    using util::Vector3<T>::x;
    using util::Vector3<T>::y;
    using util::Vector3<T>::z;
    Dimension d;
    constexpr Position3() : util::Vector3<T>(), d()
    {
    }
    constexpr Position3(util::Vector3<T> v, Dimension d) : util::Vector3<T>(v), d(d)
    {
    }
    constexpr Position3(T v, Dimension d) : util::Vector3<T>(v), d(d)
    {
    }
    constexpr Position3(T x, T y, T z, Dimension d) : util::Vector3<T>(x, y, z), d(d)
    {
    }
    template <typename U>
    constexpr explicit Position3(const Position3<U> &v)
        : util::Vector3<T>(v), d(v.d)
    {
    }
    template <typename U>
    friend void operator+(const Position3<U> &a, const Position3<U> &b);
    friend constexpr Position3 operator+(const Position3 &a, const util::Vector3<T> &b)
    {
        return Position3(static_cast<util::Vector3<T>>(a) + b, a.d);
    }
    friend constexpr Position3 operator+(const util::Vector3<T> &a, const Position3 &b)
    {
        return Position3(a + static_cast<util::Vector3<T>>(b), b.d);
    }
    friend util::Vector3<T> operator-(const Position3 &a, const Position3 &b)
    {
        return static_cast<util::Vector3<T>>(a) - static_cast<util::Vector3<T>>(b);
    }
    friend constexpr Position3 operator-(const Position3 &a, const util::Vector3<T> &b)
    {
        return Position3(static_cast<util::Vector3<T>>(a) - b, a.d);
    }
    template <typename U>
    friend void operator-(const util::Vector3<U> &a, const Position3<U> &b);
    Position3 &operator+=(const Position3 &rt) = delete;
    Position3 &operator-=(const Position3 &rt) = delete;
    Position3 &operator+=(const util::Vector3<T> &rt)
    {
        *this = *this + rt;
        return *this;
    }
    Position3 &operator-=(const util::Vector3<T> &rt)
    {
        *this = *this - rt;
        return *this;
    }
    constexpr friend bool operator==(const Position3 &a, const Position3 &b)
    {
        return static_cast<util::Vector3<T>>(a) == static_cast<util::Vector3<T>>(b) && a.d == b.d;
    }
    constexpr friend bool operator!=(const Position3 &a, const Position3 &b)
    {
        return !(a == b);
    }
};

template <typename U>
void operator+(const Position3<U> &a, const Position3<U> &b) = delete;

template <typename U>
void operator-(const util::Vector3<U> &a, const Position3<U> &b) = delete;

typedef Position3<float> Position3F;
typedef Position3<std::int32_t> Position3I32;
}
}
}

namespace std
{
template <typename T>
struct hash<programmerjake::voxels::world::Position3<T>>
{
    std::size_t operator()(const programmerjake::voxels::world::Position3<T> &v) const
    {
        return std::hash<programmerjake::voxels::util::Vector3<T>>()(v)
               + 45691U * std::hash<programmerjake::voxels::world::Dimension>()(v.d);
    }
};
}

#endif /* WORLD_POSITION_H_ */
