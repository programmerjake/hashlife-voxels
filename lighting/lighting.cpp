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
#include "lighting.h"
#include <type_traits>

namespace programmerjake
{
namespace voxels
{
namespace lighting
{
float BlockLighting::evalVertex(
    const util::Array<util::Array<util::Array<float, 3>, 3>, 3> &blockValues,
    util::Vector3I32 offset) noexcept
{
    float retval = 0;
    for(int dx = 0; dx < 2; dx++)
    {
        for(int dy = 0; dy < 2; dy++)
        {
            for(int dz = 0; dz < 2; dz++)
            {
                auto pos = offset + util::Vector3I32(dx, dy, dz);
                auto blockValue = blockValues[pos.x][pos.y][pos.z];
                if(blockValue > retval)
                    retval = blockValue;
            }
        }
    }
    return retval;
}

BlockLighting::BlockLighting(
    const util::Array<util::Array<util::Array<std::pair<LightProperties, Lighting>, 3>, 3>, 3> &
        blocksIn,
    const Lighting::GlobalProperties &globalProperties) noexcept : lightValues()
{
    constexpr int blocksSize = 3;
    util::Array<util::Array<util::Array<std::pair<LightProperties, Lighting>, blocksSize>,
                            blocksSize>,
                blocksSize> blocks = blocksIn;
    util::Array<util::Array<util::Array<bool, 3>, 3>, 3> isOpaque, setOpaque;
    for(int x = 0; x < 3; x++)
    {
        for(int y = 0; y < 3; y++)
        {
            for(int z = 0; z < 3; z++)
            {
                LightProperties &lp = std::get<0>(blocks[x][y][z]);
                isOpaque[x][y][z] = (lp.reduceValue.indirectSkylight >= Lighting::maxLight / 2);
                setOpaque[x][y][z] = true;
            }
        }
    }
    for(auto &i : blocks)
    {
        for(auto &j : i)
        {
            for(auto &lv : j)
            {
                std::get<0>(lv).emissiveValue.directSkylight = 0;
                std::get<1>(lv).directSkylight = 0;
            }
        }
    }
    for(int x = 0; x < 3; x++)
        setOpaque[x][1][1] = false;
    for(int y = 0; y < 3; y++)
        setOpaque[1][y][1] = false;
    for(int z = 0; z < 3; z++)
        setOpaque[1][1][z] = false;
    for(int dx = -1; dx <= 1; dx += 2)
    {
        for(int dy = -1; dy <= 1; dy += 2)
        {
            const int dz = 0;
            util::Vector3I32 startPos = util::Vector3I32(dx, dy, dz) + util::Vector3I32(1);
            for(util::Vector3I32 propagateDir :
                {util::Vector3I32(-dx, 0, 0), util::Vector3I32(0, -dy, 0)})
            {
                util::Vector3I32 p = startPos + propagateDir;
                if(!isOpaque[p.x][p.y][p.z])
                    setOpaque[startPos.x][startPos.y][startPos.z] = false;
            }
        }
    }
    for(int dx = -1; dx <= 1; dx += 2)
    {
        for(int dz = -1; dz <= 1; dz += 2)
        {
            const int dy = 0;
            util::Vector3I32 startPos = util::Vector3I32(dx, dy, dz) + util::Vector3I32(1);
            for(util::Vector3I32 propagateDir :
                {util::Vector3I32(-dx, 0, 0), util::Vector3I32(0, 0, -dz)})
            {
                util::Vector3I32 p = startPos + propagateDir;
                if(!isOpaque[p.x][p.y][p.z])
                    setOpaque[startPos.x][startPos.y][startPos.z] = false;
            }
        }
    }
    for(int dy = -1; dy <= 1; dy += 2)
    {
        for(int dz = -1; dz <= 1; dz += 2)
        {
            const int dx = 0;
            util::Vector3I32 startPos = util::Vector3I32(dx, dy, dz) + util::Vector3I32(1);
            for(util::Vector3I32 propagateDir :
                {util::Vector3I32(0, -dy, 0), util::Vector3I32(0, 0, -dz)})
            {
                util::Vector3I32 p = startPos + propagateDir;
                if(!isOpaque[p.x][p.y][p.z])
                    setOpaque[startPos.x][startPos.y][startPos.z] = false;
            }
        }
    }
    for(int dx = -1; dx <= 1; dx += 2)
    {
        for(int dy = -1; dy <= 1; dy += 2)
        {
            for(int dz = -1; dz <= 1; dz += 2)
            {
                setOpaque[dx + 1][dy + 1][dz + 1] = false;
            }
        }
    }
    for(int x = 0; x < 3; x++)
    {
        for(int y = 0; y < 3; y++)
        {
            for(int z = 0; z < 3; z++)
            {
                if(setOpaque[x][y][z])
                    isOpaque[x][y][z] = true;
            }
        }
    }
    for(int dx = -1; dx <= 1; dx += 2)
    {
        for(int dy = -1; dy <= 1; dy += 2)
        {
            for(int dz = -1; dz <= 1; dz += 2)
            {
                setOpaque[dx + 1][dy + 1][dz + 1] = true;
            }
        }
    }
    for(int dx = -1; dx <= 1; dx += 2)
    {
        for(int dy = -1; dy <= 1; dy += 2)
        {
            for(int dz = -1; dz <= 1; dz += 2)
            {
                util::Vector3I32 startPos = util::Vector3I32(dx, dy, dz) + util::Vector3I32(1);
                for(util::Vector3I32 propagateDir : {util::Vector3I32(-dx, 0, 0),
                                                     util::Vector3I32(0, -dy, 0),
                                                     util::Vector3I32(0, 0, -dz)})
                {
                    util::Vector3I32 p = startPos + propagateDir;
                    if(!isOpaque[p.x][p.y][p.z])
                        setOpaque[startPos.x][startPos.y][startPos.z] = false;
                }
            }
        }
    }
    for(int x = 0; x < 3; x++)
    {
        for(int y = 0; y < 3; y++)
        {
            for(int z = 0; z < 3; z++)
            {
                if(setOpaque[x][y][z])
                    isOpaque[x][y][z] = true;
            }
        }
    }
    util::Array<util::Array<util::Array<float, 3>, 3>, 3> blockValues;
    for(size_t x = 0; x < blockValues.size(); x++)
    {
        for(size_t y = 0; y < blockValues[x].size(); y++)
        {
            for(size_t z = 0; z < blockValues[x][y].size(); z++)
            {
                blockValues[x][y][z] = 0;
                if(!isOpaque[x][y][z])
                    blockValues[x][y][z] = std::get<1>(blocks[x][y][z]).toFloat(globalProperties);
            }
        }
    }
    for(int ox = 0; ox <= 1; ox++)
    {
        for(int oy = 0; oy <= 1; oy++)
        {
            for(int oz = 0; oz <= 1; oz++)
            {
                lightValues[ox][oy][oz] = evalVertex(blockValues, util::Vector3I32(ox, oy, oz));
            }
        }
    }
}
}
}
}
