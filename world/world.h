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

#ifndef WORLD_WORLD_H_
#define WORLD_WORLD_H_

#include "hashlife_world.h"
#include "dimension.h"
#include <limits>
#include <memory>

namespace programmerjake
{
namespace voxels
{
namespace world
{
class World final
{
private:
    struct PrivateAccess final
    {
        friend class World;

    private:
        PrivateAccess() = default;
    };

private:
    DimensionMap<std::shared_ptr<HashlifeWorld>> hashlifeWorlds;

private:
    const std::shared_ptr<HashlifeWorld> &getOrMakeHashlifeWorld(Dimension dimension)
    {
        auto &retval = hashlifeWorlds[dimension];
        if(!retval)
            retval = HashlifeWorld::make();
        return retval;
    }
    std::shared_ptr<HashlifeWorld> getHashlifeWorld(Dimension dimension) const noexcept
    {
        auto iter = hashlifeWorlds.find(dimension);
        if(iter == hashlifeWorlds.end())
            return nullptr;
        return std::get<1>(*iter);
    }

public:
    World(PrivateAccess) : hashlifeWorlds()
    {
        hashlifeWorlds[Dimension::overworld()] = HashlifeWorld::make();
    }
    static std::shared_ptr<World> make()
    {
        return std::make_shared<World>(PrivateAccess());
    }
    template <typename BlocksArray>
    void setBlocks(BlocksArray &&blocksArray,
                   Position3I32 worldPosition,
                   util::Vector3I32 arrayPosition,
                   util::Vector3I32 size)
    {
        getOrMakeHashlifeWorld(worldPosition.d)
            ->setBlocks(std::forward<BlocksArray>(blocksArray), worldPosition, arrayPosition, size);
    }
    template <typename BlocksArray>
    void getBlocks(BlocksArray &&blocksArray,
                   Position3I32 worldPosition,
                   util::Vector3I32 arrayPosition,
                   util::Vector3I32 size)
    {
        getOrMakeHashlifeWorld(worldPosition.d)
            ->getBlocks(std::forward<BlocksArray>(blocksArray), worldPosition, arrayPosition, size);
    }
};
}
}
}

#endif /* WORLD_WORLD_H_ */
