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
    typedef std::uint32_t ValueType;
    ValueType value;
    static constexpr int blockKindValueBitWidth =
        std::numeric_limits<ValueType>::digits - 3 * lighting::Lighting::lightBitWidth;
    static_assert(blockKindValueBitWidth >= 16, "");
    constexpr Block(BlockKind blockKind, lighting::Lighting lighting)
        : value(static_cast<ValueType>(lighting.directSkylight
                                       % (1UL << lighting::Lighting::lightBitWidth))
                | (static_cast<ValueType>(lighting.indirectSkylight
                                          % (1UL << lighting::Lighting::lightBitWidth))
                   << lighting::Lighting::lightBitWidth)
                | (static_cast<ValueType>(lighting.indirectArtificalLight
                                          % (1UL << lighting::Lighting::lightBitWidth))
                   << lighting::Lighting::lightBitWidth * 2)
                | (static_cast<ValueType>(blockKind.value % (1UL << blockKindValueBitWidth))
                   << lighting::Lighting::lightBitWidth * 3))
    {
    }
    constexpr explicit Block(ValueType value) : value(value)
    {
    }
    constexpr Block() : Block(BlockKind::empty(), lighting::Lighting())
    {
    }
    constexpr lighting::Lighting::LightValueType getDirectSkylight() const
    {
        return value % (1UL << lighting::Lighting::lightBitWidth);
    }
    constexpr lighting::Lighting::LightValueType getIndirectSkylight() const
    {
        return (value >> lighting::Lighting::lightBitWidth * 1)
               % (1UL << lighting::Lighting::lightBitWidth);
    }
    constexpr lighting::Lighting::LightValueType getIndirectArtificalLight() const
    {
        return (value >> lighting::Lighting::lightBitWidth * 2)
               % (1UL << lighting::Lighting::lightBitWidth);
    }
    constexpr lighting::Lighting getLighting() const
    {
        return lighting::Lighting(getDirectSkylight(),
                                  getIndirectSkylight(),
                                  getIndirectArtificalLight(),
                                  lighting::Lighting::makeDirectOnly);
    }
    constexpr BlockKind getBlockKind() const
    {
        return BlockKind((value >> lighting::Lighting::lightBitWidth * 3)
                         % (1UL << blockKindValueBitWidth));
    }
    friend constexpr bool operator==(Block a, Block b)
    {
        return a.value == b.value;
    }
    friend constexpr bool operator!=(Block a, Block b)
    {
        return a.value != b.value;
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

template <>
struct hash<programmerjake::voxels::block::Block>
{
    std::size_t operator()(programmerjake::voxels::block::Block v) const
    {
        return std::hash<programmerjake::voxels::block::Block::ValueType>()(v.value);
    }
};
}

#endif /* BLOCK_BLOCK_H_ */
