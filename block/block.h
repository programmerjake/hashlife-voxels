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

#ifndef BLOCK_BLOCK_H_
#define BLOCK_BLOCK_H_

#include <cstdint>
#include <utility>
#include <functional>
#include <limits>
#include "../lighting/lighting.h"

namespace programmerjake
{
namespace voxels
{
namespace block
{
struct BlockKind final
{
    typedef std::uint32_t ValueType;
    ValueType value;
    static constexpr BlockKind empty()
    {
        return {};
    }
    static constexpr BlockKind air()
    {
        return BlockKind{static_cast<ValueType>(empty().value + 1)};
    }
    static constexpr BlockKind lastPredefinedBlockKind()
    {
        return air();
    }
    constexpr BlockKind(ValueType value) : value(value)
    {
    }
    constexpr BlockKind() : value(0)
    {
    }
    constexpr explicit operator bool() const noexcept
    {
        return value != 0;
    }
    static BlockKind allocate() noexcept;
    friend constexpr bool operator==(BlockKind a, BlockKind b) noexcept
    {
        return a.value == b.value;
    }
    friend constexpr bool operator!=(BlockKind a, BlockKind b) noexcept
    {
        return a.value != b.value;
    }
};

struct BlockKindLess
{
    constexpr bool operator()(BlockKind a, BlockKind b) noexcept
    {
        return a.value < b.value;
    }
};

struct Block final
{
    static constexpr int blockKindValueBitWidth =
        std::numeric_limits<BlockKind::ValueType>::digits - 3 * lighting::Lighting::lightBitWidth;
    lighting::Lighting::LightValueType directSkylight : lighting::Lighting::lightBitWidth;
    lighting::Lighting::LightValueType indirectSkylight : lighting::Lighting::lightBitWidth;
    lighting::Lighting::LightValueType indirectArtificalLight : lighting::Lighting::lightBitWidth;
    BlockKind::ValueType blockKindValue : blockKindValueBitWidth;
    constexpr Block(BlockKind blockKind, lighting::Lighting lighting)
        : directSkylight(lighting.directSkylight),
          indirectSkylight(lighting.indirectSkylight),
          indirectArtificalLight(lighting.indirectArtificalLight),
          blockKindValue(blockKind.value)
    {
    }
    constexpr Block() : Block(BlockKind::empty(), lighting::Lighting())
    {
    }
};
}
}
}

namespace std
{
template <>
struct hash<programmerjake::voxels::block::BlockKind>
{
    std::size_t operator()(programmerjake::voxels::block::BlockKind v) const
    {
        return std::hash<programmerjake::voxels::block::BlockKind::ValueType>()(v.value);
    }
};
}

#endif /* BLOCK_BLOCK_H_ */
