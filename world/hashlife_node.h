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

#ifndef WORLD_HASHLIFE_NODE_H_
#define WORLD_HASHLIFE_NODE_H_

#include "../block/block.h"
#include <array>
#include <type_traits>

namespace programmerjake
{
namespace voxels
{
namespace world
{
struct HashlifeNode final
{
    const std::uint8_t level;
    typedef std::array<std::array<std::array<block::Block, 2>, 2>, 2> BlocksArray;
    static_assert(std::is_trivially_destructible<BlocksArray>::value, "");
    union
    {
        BlocksArray blocks;
    };
#error finish
};
}
}
}

#endif /* WORLD_HASHLIFE_NODE_H_ */
