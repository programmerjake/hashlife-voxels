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
#include "../util/enum.h"
#include "../util/vector.h"

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
    constexpr explicit Block(BlockKind blockKind) : Block(blockKind, lighting::Lighting())
    {
    }
    constexpr Block() : Block(BlockKind::empty(), lighting::Lighting::makeSkyLighting())
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
    constexpr lighting::Lighting getLightingIfNotEmpty() const
    {
        return getBlockKind() == BlockKind::empty() ? lighting::Lighting() : getLighting();
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

struct BlockSummary final
{
    bool areAllBlocksRenderedLikeAir : 1;
    bool areAllBlocksRenderedLikeBedrock : 1;
    constexpr bool rendersAnything() const noexcept
    {
        return !areAllBlocksRenderedLikeAir && !areAllBlocksRenderedLikeBedrock;
    }
    constexpr BlockSummary() noexcept : areAllBlocksRenderedLikeAir(false),
                                        areAllBlocksRenderedLikeBedrock(false)
    {
    }
    constexpr BlockSummary(bool areAllBlocksRenderedLikeAir,
                           bool areAllBlocksRenderedLikeBedrock) noexcept
        : areAllBlocksRenderedLikeAir(areAllBlocksRenderedLikeAir),
          areAllBlocksRenderedLikeBedrock(areAllBlocksRenderedLikeBedrock)
    {
    }
    static constexpr BlockSummary makeForEmptyBlockKind() noexcept
    {
        return BlockSummary(
            true,
            true); // all rendering flags set because we don't render faces against an empty block
    }
    constexpr BlockSummary operator+(const BlockSummary &rt) const noexcept
    {
        return BlockSummary(areAllBlocksRenderedLikeAir && rt.areAllBlocksRenderedLikeAir,
                            areAllBlocksRenderedLikeBedrock && rt.areAllBlocksRenderedLikeBedrock);
    }
    BlockSummary &operator+=(const BlockSummary &rt) noexcept
    {
        *this = operator+(rt);
        return *this;
    }
};

enum class BlockFace : std::uint8_t
{
    XAxis = 0,
    YAxis = 2,
    ZAxis = 4,
    NegativeDirection = 0,
    PositiveDirection = 1,
    NX = XAxis | NegativeDirection,
    PX = XAxis | PositiveDirection,
    NY = YAxis | NegativeDirection,
    PY = YAxis | PositiveDirection,
    NZ = ZAxis | NegativeDirection,
    PZ = ZAxis | PositiveDirection,
    DEFINE_ENUM_LIMITS(NX, PZ)
};

constexpr BlockFace reverse(BlockFace blockFace) noexcept
{
    return static_cast<BlockFace>(static_cast<std::uint8_t>(blockFace)
                                  ^ (static_cast<std::uint8_t>(BlockFace::PositiveDirection)
                                     ^ static_cast<std::uint8_t>(BlockFace::NegativeDirection)));
}

constexpr util::Vector3I32 getDirection(BlockFace blockFace) noexcept
{
    return util::Vector3I32(blockFace == BlockFace::NX ? -1 : blockFace == BlockFace::PX ? 1 : 0,
                            blockFace == BlockFace::NY ? -1 : blockFace == BlockFace::PY ? 1 : 0,
                            blockFace == BlockFace::NZ ? -1 : blockFace == BlockFace::PZ ? 1 : 0);
}
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
