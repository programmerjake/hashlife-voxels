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

#ifndef WORLD_DIMENSION_H_
#define WORLD_DIMENSION_H_

#include <cstdint>
#include <functional>

namespace programmerjake
{
namespace voxels
{
namespace world
{
template <typename = void>
struct DimensionType final
{
    typedef std::uint8_t ValueType;
    ValueType value = 0;
    static const DimensionType Overworld;
    static const DimensionType Nether;

private:
    static const DimensionType LastPredefinedDimension;

public:
    static DimensionType allocate(float zeroBrightnessLevel) noexcept;
    float getZeroBrightnessLevel() const noexcept;
};

template <typename T>
constexpr DimensionType<T> DimensionType<T>::Overworld{};

template <typename T>
constexpr DimensionType<T> DimensionType<T>::Nether{Overworld.value + 1};

template <typename T>
constexpr DimensionType<T> DimensionType<T>::LastPredefinedDimension{Nether.value};

typedef DimensionType<> Dimension;

extern template DimensionType<void> DimensionType<void>::allocate(float zeroBrightnessLevel) noexcept;
extern template float DimensionType<void>::getZeroBrightnessLevel() const noexcept;
}
}
}

namespace std
{
template <>
struct hash<programmerjake::voxels::world::Dimension>
{
    std::size_t operator()(programmerjake::voxels::world::Dimension v) const
    {
        return std::hash<programmerjake::voxels::world::Dimension::ValueType>()(v.value);
    }
};
}

#endif /* WORLD_DIMENSION_H_ */
