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

namespace programmerjake
{
namespace voxels
{
namespace block
{
template <typename = void>
struct BlockKindType final
{
    typedef std::uint32_t ValueType;
    ValueType value = 0;
    static const BlockKindType Empty;
    static const BlockKindType Air;

private:
    static const BlockKindType LastPredefinedBlockKind;

public:
    constexpr explicit operator bool() const noexcept
    {
        return value != 0;
    }
    static BlockKindType allocate() noexcept;
    friend constexpr bool operator==(BlockKindType a, BlockKindType b) noexcept
    {
        return a.value == b.value;
    }
    friend constexpr bool operator!=(BlockKindType a, BlockKindType b) noexcept
    {
        return a.value != b.value;
    }
};

template <typename T>
constexpr BlockKindType<T> BlockKindType<T>::Empty{};

template <typename T>
constexpr BlockKindType<T> BlockKindType<T>::Air{Empty.value + 1};

template <typename T>
constexpr BlockKindType<T> BlockKindType<T>::LastPredefinedBlockKind{Air.value};

extern template BlockKindType<> BlockKindType<void>::allocate() noexcept;

typedef BlockKindType<> BlockKind;

struct BlockKindLess
{
    constexpr bool operator()(BlockKind a, BlockKind b) noexcept
    {
        return a.value < b.value;
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
