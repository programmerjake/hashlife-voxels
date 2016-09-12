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
#include "../util/constexpr_array.h"
#include <type_traits>
#include <list>
#include <cstdint>

namespace programmerjake
{
namespace voxels
{
namespace world
{
class HashlifeNonleafNode;
class HashlifeLeafNode;
class HashlifeGarbageCollectedHashtable;
class HashlifeNodeBase
{
    friend class HashlifeNonleafNode;
    friend class HashlifeLeafNode;
    friend class HashlifeGarbageCollectedHashtable;

private:
    HashlifeNodeBase *hashNext = nullptr;
    struct GarbageCollectState final
    {
        struct StateMarked final
        {
            HashlifeNodeBase *worklistNext = nullptr;
        };
        struct StateUnmarked final
        {
            HashlifeNodeBase *collectPendingNodeQueueNext = nullptr;
            HashlifeNodeBase *collectPendingNodeQueuePrev = nullptr;
            constexpr bool isInCollectPendingNodeQueue(
                HashlifeNodeBase *collectPendingNodeQueueHead)
            {
                return collectPendingNodeQueueHead != nullptr
                       && (&collectPendingNodeQueueHead->garbageCollectState.stateUnmarked == this
                           || collectPendingNodeQueuePrev != nullptr);
            }
            void removeFromCollectPendingNodeQueue(
                HashlifeNodeBase *&collectPendingNodeQueueHead,
                HashlifeNodeBase *&collectPendingNodeQueueTail) noexcept
            {
                constexprAssert(isInCollectPendingNodeQueue(collectPendingNodeQueueHead));
                if(collectPendingNodeQueueNext)
                    collectPendingNodeQueueNext->garbageCollectState.stateUnmarked
                        .collectPendingNodeQueuePrev = collectPendingNodeQueuePrev;
                else
                    collectPendingNodeQueueTail = collectPendingNodeQueuePrev;
                if(collectPendingNodeQueuePrev)
                    collectPendingNodeQueuePrev->garbageCollectState.stateUnmarked
                        .collectPendingNodeQueueNext = collectPendingNodeQueueNext;
                else
                    collectPendingNodeQueueHead = collectPendingNodeQueueNext;
                collectPendingNodeQueueNext = nullptr;
                collectPendingNodeQueuePrev = nullptr;
            }
            void addToCollectPendingNodeQueue(
                HashlifeNodeBase *node,
                HashlifeNodeBase *&collectPendingNodeQueueHead,
                HashlifeNodeBase *&collectPendingNodeQueueTail) noexcept
            {
                constexprAssert(!isInCollectPendingNodeQueue(collectPendingNodeQueueHead));
                constexprAssert(&node->garbageCollectState.stateUnmarked == this);
                collectPendingNodeQueueNext = nullptr;
                collectPendingNodeQueuePrev = collectPendingNodeQueueTail;
                if(collectPendingNodeQueueTail)
                    collectPendingNodeQueueTail->garbageCollectState.stateUnmarked
                        .collectPendingNodeQueueNext = node;
                else
                    collectPendingNodeQueueHead = node;
                collectPendingNodeQueueTail = node;
            }
        };
#if 0
        StateMarked stateMarked;
        StateUnmarked stateUnmarked;
#else
        union
        {
            StateMarked stateMarked;
            StateUnmarked stateUnmarked;
        };
#endif
        bool markFlag;
        constexpr GarbageCollectState() : stateUnmarked(), markFlag(false)
        {
        }
    };
    GarbageCollectState garbageCollectState;

public:
    static constexpr std::int32_t levelSize = 2;
    typedef std::uint8_t LevelType;
    static constexpr LevelType maxLevel = 32 - 2;
    const LevelType level;
    const block::BlockSummary blockSummary;
    static constexpr bool isLeaf(LevelType level)
    {
        return level == 0;
    }
    constexpr bool isLeaf() const
    {
        return isLeaf(level);
    }
    static constexpr std::uint32_t getSize(LevelType level)
    {
        static_assert(levelSize == 2, "");
        return 2UL << level;
    }
    static constexpr int getLogBase2OfSize(LevelType level)
    {
        static_assert(levelSize == 2, "");
        return 1 + level;
    }
    constexpr std::uint32_t getSize() const
    {
        return getSize(level);
    }
    static constexpr std::int32_t getHalfSize(LevelType level)
    {
        static_assert(levelSize == 2, "");
        return 1L << level;
    }
    constexpr std::int32_t getHalfSize() const
    {
        return getHalfSize(level);
    }
    static constexpr std::int32_t getQuarterSize(LevelType level)
    {
        static_assert(levelSize == 2, "");
        return (constexprAssert(level >= 1), 1L << (level - 1));
    }
    constexpr std::int32_t getQuarterSize() const
    {
        return getQuarterSize(level);
    }
    static constexpr std::int32_t getEighthSize(LevelType level)
    {
        static_assert(levelSize == 2, "");
        return (constexprAssert(level >= 2), 1L << (level - 2));
    }
    constexpr std::int32_t getEighthSize() const
    {
        return getEighthSize(level);
    }
    constexpr bool isPositionInside(std::int32_t position) const
    {
        return position >= -getHalfSize() && position < getHalfSize();
    }
    constexpr bool isPositionInside(util::Vector3I32 position) const
    {
        return isPositionInside(position.x) && isPositionInside(position.y)
               && isPositionInside(position.z);
    }
    constexpr std::uint32_t getIndex(std::int32_t position) const
    {
        return (constexprAssert(isPositionInside(position)), position >= 0 ? 1 : 0);
    }
    constexpr std::int32_t getChildPosition(std::int32_t position) const
    {
        return (constexprAssert(isPositionInside(position)),
                position >= 0 ? position - getQuarterSize() : position + getQuarterSize());
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
    constexpr const HashlifeNodeBase *get(util::Vector3I32 position, LevelType returnedLevel) const;
    HashlifeNodeBase *get(util::Vector3I32 position, LevelType returnedLevel)
    {
        return const_cast<HashlifeNodeBase *>(
            static_cast<const HashlifeNodeBase *>(this)->get(position, returnedLevel));
    }
    constexpr bool operator==(const HashlifeNodeBase &rt) const;
    std::size_t hash() const;
    HashlifeNodeBase *duplicate() const &;
    HashlifeNodeBase *duplicate() && ;
    static void free(HashlifeNodeBase *node) noexcept;
    friend constexpr HashlifeLeafNode *getAsLeaf(HashlifeNodeBase *node) noexcept;
    friend constexpr HashlifeNonleafNode *getAsNonleaf(HashlifeNodeBase *node) noexcept;
    friend constexpr const HashlifeLeafNode *getAsLeaf(const HashlifeNodeBase *node) noexcept;
    friend constexpr const HashlifeNonleafNode *getAsNonleaf(const HashlifeNodeBase *node) noexcept;
    friend constexpr HashlifeLeafNode &getAsLeaf(HashlifeNodeBase &node) noexcept
    {
        return *getAsLeaf(&node);
    }
    friend constexpr HashlifeNonleafNode &getAsNonleaf(HashlifeNodeBase &node) noexcept
    {
        return *getAsNonleaf(&node);
    }
    friend constexpr const HashlifeLeafNode &getAsLeaf(const HashlifeNodeBase &node) noexcept
    {
        return *getAsLeaf(&node);
    }
    friend constexpr const HashlifeNonleafNode &getAsNonleaf(const HashlifeNodeBase &node) noexcept
    {
        return *getAsNonleaf(&node);
    }

private:
    constexpr explicit HashlifeNodeBase(LevelType level, const block::BlockSummary &blockSummary)
        : level((constexprAssert(level <= maxLevel), level))
    {
    }
};

class HashlifeNonleafNode final : public HashlifeNodeBase
{
public:
    struct FutureState final
    {
        static constexpr std::uint32_t getStepSizeInGenerations(LevelType level)
        {
            return (constexprAssert(level >= 1),
                    static_cast<LevelType>(level - 1)
                            > block::BlockStepGlobalState::log2OfStepSizeInGenerations ?
                        block::BlockStepGlobalState::stepSizeInGenerations :
                        1UL << (level - 1));
        }
        HashlifeNodeBase *node;
        block::BlockStepGlobalState globalState;
        typedef util::Array<util::Array<util::Array<block::BlockStepExtraActions, levelSize>,
                                        levelSize>,
                            levelSize> ActionsArray;
        ActionsArray actions;
        constexpr FutureState() : node(nullptr), globalState(), actions()
        {
        }
        FutureState(HashlifeNodeBase *node,
                    block::BlockStepGlobalState globalState,
                    ActionsArray actions)
            : node(node), globalState(std::move(globalState)), actions(std::move(actions))
        {
        }
        explicit FutureState(block::BlockStepGlobalState globalState)
            : node(nullptr), globalState(std::move(globalState)), actions()
        {
        }
    };
    typedef util::Array<util::Array<util::Array<HashlifeNodeBase *, levelSize>, levelSize>,
                        levelSize> ChildNodesArray;

private:
    const ChildNodesArray childNodes;

public:
    HashlifeNodeBase *getChildNode(util::Vector3U32 index) const
    {
        return (constexprAssert(!isLeaf()), childNodes[index.x][index.y][index.z]);
    }
    HashlifeNodeBase *getChildNode(util::Vector3I32 index) const
    {
        return getChildNode(util::Vector3U32(index));
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
        : HashlifeNodeBase((constexprAssert(nxnynz), nxnynz->level + 1),
                           nxnynz->blockSummary + nxnypz->blockSummary + nxpynz->blockSummary
                               + nxpypz->blockSummary + pxnynz->blockSummary + pxnypz->blockSummary
                               + pxpynz->blockSummary + pxpypz->blockSummary),
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
        static_assert(levelSize == 2, "");
    }
    constexpr explicit HashlifeNonleafNode(const ChildNodesArray &childNodes)
        : HashlifeNonleafNode(childNodes[0][0][0],
                              childNodes[0][0][1],
                              childNodes[0][1][0],
                              childNodes[0][1][1],
                              childNodes[1][0][0],
                              childNodes[1][0][1],
                              childNodes[1][1][0],
                              childNodes[1][1][1])
    {
        static_assert(levelSize == 2, "");
    }
    bool operator!=(const HashlifeNonleafNode &rt) const
    {
        return !operator==(rt);
    }
    bool operator==(const HashlifeNonleafNode &rt) const
    {
        return (constexprAssert(!isLeaf() && !rt.isLeaf()),
                level == rt.level && childNodes == rt.childNodes);
    }
    std::size_t hash() const
    {
        constexprAssert(!isLeaf());
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
    typedef util::Array<util::Array<util::Array<block::Block, levelSize>, levelSize>, levelSize>
        BlocksArray;

private:
    const BlocksArray blocks;

public:
    constexpr block::Block getBlock(util::Vector3I32 index) const
    {
        return getBlock(util::Vector3U32(index));
    }
    constexpr block::Block getBlock(util::Vector3U32 index) const
    {
        return (constexprAssert(isLeaf()), blocks[index.x][index.y][index.z]);
    }
    constexpr HashlifeLeafNode(block::Block nxnynz,
                               block::Block nxnypz,
                               block::Block nxpynz,
                               block::Block nxpypz,
                               block::Block pxnynz,
                               block::Block pxnypz,
                               block::Block pxpynz,
                               block::Block pxpypz,
                               const block::BlockSummary &blockSummary)
        : HashlifeNodeBase(0, blockSummary),
          blocks{
              nxnynz, nxnypz, nxpynz, nxpypz, pxnynz, pxnypz, pxpynz, pxpypz,
          }
    {
        static_assert(levelSize == 2, "");
    }
    HashlifeLeafNode(block::Block nxnynz,
                     block::Block nxnypz,
                     block::Block nxpynz,
                     block::Block nxpypz,
                     block::Block pxnynz,
                     block::Block pxnypz,
                     block::Block pxpynz,
                     block::Block pxpypz)
        : HashlifeLeafNode(nxnynz,
                           nxnypz,
                           nxpynz,
                           nxpypz,
                           pxnynz,
                           pxnypz,
                           pxpynz,
                           pxpypz,
                           (block::BlockDescriptor::getBlockSummary(nxnynz.getBlockKind())
                            + block::BlockDescriptor::getBlockSummary(nxnypz.getBlockKind())
                            + block::BlockDescriptor::getBlockSummary(nxpynz.getBlockKind())
                            + block::BlockDescriptor::getBlockSummary(nxpypz.getBlockKind())
                            + block::BlockDescriptor::getBlockSummary(pxnynz.getBlockKind())
                            + block::BlockDescriptor::getBlockSummary(pxnypz.getBlockKind())
                            + block::BlockDescriptor::getBlockSummary(pxpynz.getBlockKind())
                            + block::BlockDescriptor::getBlockSummary(pxpypz.getBlockKind())))
    {
    }
    constexpr HashlifeLeafNode(const BlocksArray &blocks, const block::BlockSummary &blockSummary)
        : HashlifeNodeBase(0, blockSummary), blocks(blocks)
    {
    }
    HashlifeLeafNode(const BlocksArray &blocks)
        : HashlifeLeafNode(blocks[0][0][0],
                           blocks[0][0][1],
                           blocks[0][1][0],
                           blocks[0][1][1],
                           blocks[1][0][0],
                           blocks[1][0][1],
                           blocks[1][1][0],
                           blocks[1][1][1])
    {
    }
    bool operator!=(const HashlifeLeafNode &rt) const
    {
        return !operator==(rt);
    }
    bool operator==(const HashlifeLeafNode &rt) const
    {
        return (constexprAssert(isLeaf() && rt.isLeaf()), blocks == rt.blocks);
    }
    std::size_t hash() const
    {
        constexprAssert(isLeaf());
        std::size_t retval = 0;
        for(auto &i : blocks)
            for(auto &j : i)
                for(auto &block : j)
                    retval = retval * 8191 ^ (retval >> 3) ^ std::hash<block::Block>()(block);
        return retval;
    }
};

constexpr HashlifeLeafNode *getAsLeaf(HashlifeNodeBase *node) noexcept
{
    return (constexprAssert(node->isLeaf()), static_cast<HashlifeLeafNode *>(node));
}

constexpr HashlifeNonleafNode *getAsNonleaf(HashlifeNodeBase *node) noexcept
{
    return (constexprAssert(!node->isLeaf()), static_cast<HashlifeNonleafNode *>(node));
}

constexpr const HashlifeLeafNode *getAsLeaf(const HashlifeNodeBase *node) noexcept
{
    return (constexprAssert(node->isLeaf()), static_cast<const HashlifeLeafNode *>(node));
}

constexpr const HashlifeNonleafNode *getAsNonleaf(const HashlifeNodeBase *node) noexcept
{
    return (constexprAssert(!node->isLeaf()), static_cast<const HashlifeNonleafNode *>(node));
}

constexpr block::Block HashlifeNodeBase::get(util::Vector3I32 position) const
{
    return isLeaf() ? getAsLeaf(this)->getBlock(getIndex(position)) :
                      getAsNonleaf(this)
                          ->getChildNode(getIndex(position))
                          ->get(getChildPosition(position));
}

constexpr const HashlifeNodeBase *HashlifeNodeBase::get(util::Vector3I32 position,
                                                        LevelType returnedLevel) const
{
    return (constexprAssert(level >= returnedLevel),
            level == returnedLevel ? this : getAsNonleaf(this)
                                                ->getChildNode(getIndex(position))
                                                ->get(getChildPosition(position), returnedLevel));
}

constexpr bool HashlifeNodeBase::operator==(const HashlifeNodeBase &rt) const
{
    return level == rt.level && (isLeaf() ? getAsLeaf(this)->operator==(getAsLeaf(rt)) :
                                            getAsNonleaf(this)->operator==(getAsNonleaf(rt)));
}

inline std::size_t HashlifeNodeBase::hash() const
{
    if(isLeaf())
        return getAsLeaf(this)->hash();
    else
        return getAsNonleaf(this)->hash();
}

inline HashlifeNodeBase *HashlifeNodeBase::duplicate() const &
{
    if(isLeaf())
        return new HashlifeLeafNode(*getAsLeaf(this));
    else
        return new HashlifeNonleafNode(*getAsNonleaf(this));
}

inline HashlifeNodeBase *HashlifeNodeBase::duplicate() &&
{
    if(isLeaf())
        return new HashlifeLeafNode(std::move(*getAsLeaf(this)));
    else
        return new HashlifeNonleafNode(std::move(*getAsNonleaf(this)));
}

inline void HashlifeNodeBase::free(HashlifeNodeBase *node) noexcept
{
    if(node)
    {
        if(node->isLeaf())
        {
            delete getAsLeaf(node);
        }
        else
        {
            delete getAsNonleaf(node);
        }
    }
}
}
}
}

namespace std
{
template <>
struct hash<programmerjake::voxels::world::HashlifeNodeBase>
{
    std::size_t operator()(const programmerjake::voxels::world::HashlifeNodeBase &v) const
    {
        return v.hash();
    }
};
}

#endif /* WORLD_HASHLIFE_NODE_H_ */
