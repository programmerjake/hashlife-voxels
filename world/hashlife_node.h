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
#include "../util/vector.h"
#include "../util/optional.h"
#include "../block/block_descriptor.h"
#include "../util/constexpr_assert.h"
#include <array>
#include <type_traits>
#include <list>

namespace programmerjake
{
namespace voxels
{
namespace world
{
class HashlifeNonleafNode;
class HashlifeLeafNode;
class HashlifeNodeBase
{
    friend class HashlifeNonleafNode;
    friend class HashlifeLeafNode;

public:
    typedef std::uint8_t LevelType;
    const LevelType level;
    constexpr std::uint32_t getSize() const
    {
        return 2UL << level;
    }
    constexpr std::int32_t getHalfSize() const
    {
        return 1L << level;
    }
    constexpr std::int32_t getQuarterSize() const
    {
        return (constexprAssert(level > 0), 1L << (level - 1));
    }
    constexpr std::uint32_t getIndex(std::int32_t position) const
    {
        return (constexprAssert(position >= -getHalfSize() && position < getHalfSize()),
                position > 0 ? 1 : 0);
    }
    constexpr std::int32_t getChildPosition(std::int32_t position) const
    {
        return (constexprAssert(position >= -getHalfSize() && position < getHalfSize()),
                position > 0 ? position - getQuarterSize() : position + getQuarterSize());
    }
    constexpr util::Vector3U32 getIndex(util::Vector3I32 position) const
    {
        return util::Vector3U32(getIndex(position.x), getIndex(position.y), getIndex(position.z));
    }
    constexpr util::Vector3I32 getChildPosition(util::Vector3I32 position) const
    {
        return util::Vector3I32(getChildPosition(position.x),
                                getChildPosition(position.y),
                                getChildPosition(position.z));
    }
    constexpr bool operator!=(const HashlifeNodeBase &rt) const
    {
        return !operator==(rt);
    }
    constexpr block::Block get(util::Vector3I32 position) const;
    constexpr bool operator==(const HashlifeNodeBase &rt) const;
    std::size_t hash() const;

private:
    constexpr explicit HashlifeNodeBase(LevelType level) : level(level)
    {
    }
};

class HashlifeNonleafNode final : public HashlifeNodeBase
{
public:
    struct FutureState final
    {
        const HashlifeNodeBase *node;
        util::Optional<std::list<block::BlockStepExtraAction>> actions;
        constexpr FutureState() : node(nullptr), actions()
        {
        }
        FutureState(HashlifeNodeBase *node,
                    util::Optional<std::list<block::BlockStepExtraAction>> actions)
            : node(node), actions(std::move(actions))
        {
        }
    };
    typedef std::array<std::array<std::array<HashlifeNodeBase *, 2>, 2>, 2> ChildNodesArray;

public:
    const ChildNodesArray childNodes;
    constexpr HashlifeNodeBase *checkChildNode(HashlifeNodeBase *childNode) const
    {
        return (constexprAssert(childNode && childNode->level == level - 1), childNode);
    }
    constexpr static HashlifeNodeBase *getChildNode(util::Vector3U32 index) const
    {
        return checkChildNode(childNodes[index.x][index.y][index.z]);
    }
    FutureState futureState; // ignored for operator == and hash
    constexpr HashlifeNonleafNode(HashlifeNodeBase *nxnynz,
                                  HashlifeNodeBase *nxnypz,
                                  HashlifeNodeBase *nxpynz,
                                  HashlifeNodeBase *nxpypz,
                                  HashlifeNodeBase *pxnynz,
                                  HashlifeNodeBase *pxnypz,
                                  HashlifeNodeBase *pxpynz,
                                  HashlifeNodeBase *pxpypz)
        : HashlifeNodeBase((constexprAssert(nxnynz), nxnynz->level + 1)),
          childNodes{
              (constexprAssert(nxnynz && nxnynz->level + 1 == level), nxnynz),
              (constexprAssert(nxnypz && nxnypz->level + 1 == level), nxnypz),
              (constexprAssert(nxpynz && nxpynz->level + 1 == level), nxpynz),
              (constexprAssert(nxpypz && nxpypz->level + 1 == level), nxpypz),
              (constexprAssert(pxnynz && pxnynz->level + 1 == level), pxnynz),
              (constexprAssert(pxnypz && pxnypz->level + 1 == level), pxnypz),
              (constexprAssert(pxpynz && pxpynz->level + 1 == level), pxpynz),
              (constexprAssert(pxpypz && pxpypz->level + 1 == level), pxpypz),
          },
          futureState()
    {
    }
    HashlifeNonleafNode(HashlifeNodeBase *nxnynz,
                        HashlifeNodeBase *nxnypz,
                        HashlifeNodeBase *nxpynz,
                        HashlifeNodeBase *nxpypz,
                        HashlifeNodeBase *pxnynz,
                        HashlifeNodeBase *pxnypz,
                        HashlifeNodeBase *pxpynz,
                        HashlifeNodeBase *pxpypz,
                        FutureState futureState)
        : HashlifeNodeBase((constexprAssert(nxnynz), nxnynz->level + 1)),
          childNodes{
              (constexprAssert(nxnynz && nxnynz->level + 1 == level), nxnynz),
              (constexprAssert(nxnypz && nxnypz->level + 1 == level), nxnypz),
              (constexprAssert(nxpynz && nxpynz->level + 1 == level), nxpynz),
              (constexprAssert(nxpypz && nxpypz->level + 1 == level), nxpypz),
              (constexprAssert(pxnynz && pxnynz->level + 1 == level), pxnynz),
              (constexprAssert(pxnypz && pxnypz->level + 1 == level), pxnypz),
              (constexprAssert(pxpynz && pxpynz->level + 1 == level), pxpynz),
              (constexprAssert(pxpypz && pxpypz->level + 1 == level), pxpypz),
          },
          futureState((constexprAssert(futureState.node && futureState.node->level + 1 == level),
                       std::move(futureState)))
    {
    }
    constexpr bool operator!=(const HashlifeNonleafNode &rt) const
    {
        return !operator==(rt);
    }
    constexpr bool operator==(const HashlifeNonleafNode &rt) const
    {
        return (constexprAssert(level > 0 && rt.level > 0),
                level == rt.level && childNodes == rt.childNodes);
    }
    std::size_t hash() const
    {
        constexprAssert(level > 0);
        std::size_t retval = level;
        for(auto &i : childNodes)
            for(auto &j : i)
                for(auto childNode : j)
                    retval =
                        retval * 8191 ^ (retval >> 3) ^ std::hash<HashlifeNodeBase *>()(childNode);
        return retval;
    }
};

class HashlifeLeafNode final : public HashlifeNodeBase
{
public:
    typedef std::array<std::array<std::array<block::Block, 2>, 2>, 2> BlocksArray;
    const BlocksArray blocks;
    constexpr block::Block getBlock(util::Vector3U32 index) const
    {
        return blocks[index.x][index.y][index.z];
    }
    constexpr HashlifeLeafNode(block::Block nxnynz,
                               block::Block nxnypz,
                               block::Block nxpynz,
                               block::Block nxpypz,
                               block::Block pxnynz,
                               block::Block pxnypz,
                               block::Block pxpynz,
                               block::Block pxpypz)
        : HashlifeNodeBase(0),
          blocks{
              nxnynz, nxnypz, nxpynz, nxpypz, pxnynz, pxnypz, pxpynz, pxpypz,
          }
    {
    }
    constexpr bool operator!=(const HashlifeLeafNode &rt) const
    {
        return !operator==(rt);
    }
    constexpr bool operator==(const HashlifeLeafNode &rt) const
    {
        return (constexprAssert(level == 0 && rt.level == 0), blocks == rt.blocks);
    }
    std::size_t hash() const
    {
        constexprAssert(level == 0);
        std::size_t retval = 0;
        for(auto &i : blocks)
            for(auto &j : i)
                for(auto &block : j)
                    retval = retval * 8191 ^ (retval >> 3) ^ std::hash<block::Block>()(block);
        return retval;
    }
};

constexpr block::Block HashlifeNodeBase::get(util::Vector3I32 position) const
{
    return level == 0 ? static_cast<const HashlifeLeafNode *>(this)->getBlock(getIndex(position)) :
                        static_cast<const HashlifeNonleafNode *>(this)
                            ->getChildNode(getIndex(position))
                            ->get(getChildPosition(position));
}

constexpr bool HashlifeNodeBase::operator==(const HashlifeNodeBase &rt) const
{
    return level == rt.level && (level == 0 ?
                                     static_cast<const HashlifeLeafNode *>(this)->operator==(
                                         static_cast<const HashlifeLeafNode &>(rt)) :
                                     static_cast<const HashlifeNonleafNode *>(this)->operator==(
                                         static_cast<const HashlifeNonleafNode &>(rt)));
}

inline std::size_t HashlifeNodeBase::hash() const
{
    if(level == 0)
        return static_cast<const HashlifeLeafNode *>(this)->hash();
    else
        return static_cast<const HashlifeNonleafNode *>(this)->hash();
}
}
}
}

namespace std
{
struct hash<programmerjake::voxels::world::HashlifeNodeBase>
{
    std::size_t operator()(const programmerjake::voxels::world::HashlifeNodeBase &v) const
    {
        return v.hash();
    }
};
}

#endif /* WORLD_HASHLIFE_NODE_H_ */
