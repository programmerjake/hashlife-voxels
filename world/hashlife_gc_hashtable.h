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

#ifndef WORLD_HASHLIFE_GC_HASHTABLE_H_
#define WORLD_HASHLIFE_GC_HASHTABLE_H_

#include "hashlife_node.h"
#include "../util/hash.h"
#include "../util/constexpr_assert.h"
#include <vector>
#include "../util/constexpr_array.h"
#include <memory>

namespace programmerjake
{
namespace voxels
{
namespace world
{
class HashlifeGarbageCollectedHashtable final
{
    HashlifeGarbageCollectedHashtable(const HashlifeGarbageCollectedHashtable &) = delete;
    HashlifeGarbageCollectedHashtable &operator=(const HashlifeGarbageCollectedHashtable &) =
        delete;

private:
    static constexpr std::size_t bucketCount = 1UL << 17;
    util::Array<HashlifeNodeBase *, bucketCount> &buckets;
    HashlifeNodeBase *canonicalEmptyNodes[HashlifeNodeBase::maxLevel + 1];
    std::size_t nodeCount;
    HashlifeNodeBase *collectPendingNodeQueueHead;
    HashlifeNodeBase *collectPendingNodeQueueTail;

private:
    template <typename T, typename... Args>
    static void construct(T &v, Args &&... args)
    {
        ::new(static_cast<void *>(std::addressof(v))) T(std::forward<Args>(args)...);
    }
    template <typename T>
    static void destruct(T &v)
    {
        v.~T();
    }
    static std::size_t getBucketIndex(const HashlifeNodeBase &node)
    {
        return util::hashFinalize(std::hash<HashlifeNodeBase>()(node)) % bucketCount;
    }
    template <typename NodeType>
    HashlifeNodeBase *findOrAddNodeImplementation(NodeType &&nodeIn)
    {
        constexprAssert(!std::is_rvalue_reference<NodeType &&>::value
                        || !nodeIn.garbageCollectState.stateUnmarked.isInCollectPendingNodeQueue(
                               collectPendingNodeQueueHead));
        HashlifeNodeBase **pBucket = &buckets[getBucketIndex(nodeIn)];
        for(auto ppNode = pBucket; *ppNode; ppNode = &(*ppNode)->hashNext)
        {
            if(**ppNode == nodeIn)
            {
                auto pNode = *ppNode;
                *ppNode = pNode->hashNext;
                pNode->hashNext = *pBucket;
                *pBucket = pNode;
                constexprAssert(!pNode->garbageCollectState.markFlag);
                if(pNode->garbageCollectState.stateUnmarked.isInCollectPendingNodeQueue(
                       collectPendingNodeQueueHead))
                    pNode->garbageCollectState.stateUnmarked.removeFromCollectPendingNodeQueue(
                        collectPendingNodeQueueHead, collectPendingNodeQueueTail);
                return pNode;
            }
        }
        auto newNode = std::forward<NodeType>(nodeIn).duplicate();
        constexprAssert(!newNode->garbageCollectState.markFlag);
        constexprAssert(!newNode->garbageCollectState.stateUnmarked.isInCollectPendingNodeQueue(
            collectPendingNodeQueueHead));
        nodeCount++;
        newNode->hashNext = *pBucket;
        *pBucket = newNode;
        return newNode;
    }
    void unlinkAndFreeNode(HashlifeNodeBase *node) noexcept;

public:
    HashlifeGarbageCollectedHashtable();
    ~HashlifeGarbageCollectedHashtable();
    HashlifeNodeBase *findOrAddNode(const HashlifeNodeBase &nodeIn)
    {
        return findOrAddNodeImplementation(nodeIn);
    }
    HashlifeNodeBase *findOrAddNode(HashlifeNodeBase &&nodeIn)
    {
        return findOrAddNodeImplementation(std::move(nodeIn));
    }
    HashlifeNodeBase *getCanonicalEmptyNode(HashlifeNodeBase::LevelType level)
    {
        constexprAssert(level <= HashlifeNodeBase::maxLevel);
        if(!canonicalEmptyNodes[level])
        {
            if(HashlifeNodeBase::isLeaf(level))
            {
                canonicalEmptyNodes[level] = findOrAddNode(HashlifeLeafNode(block::Block(),
                                                                            block::Block(),
                                                                            block::Block(),
                                                                            block::Block(),
                                                                            block::Block(),
                                                                            block::Block(),
                                                                            block::Block(),
                                                                            block::Block()));
            }
            else
            {
                auto previousCanonicalEmptyNode = getCanonicalEmptyNode(level - 1);
                canonicalEmptyNodes[level] =
                    findOrAddNode(HashlifeNonleafNode(previousCanonicalEmptyNode,
                                                      previousCanonicalEmptyNode,
                                                      previousCanonicalEmptyNode,
                                                      previousCanonicalEmptyNode,
                                                      previousCanonicalEmptyNode,
                                                      previousCanonicalEmptyNode,
                                                      previousCanonicalEmptyNode,
                                                      previousCanonicalEmptyNode));
            }
        }
        return canonicalEmptyNodes[level];
    }

private:
    void markNode(HashlifeNodeBase *node, HashlifeNodeBase *&worklistHead);
    void freeNodesFromCollectPendingNodeQueue(std::size_t &numberOfNodesLeftToCollect) noexcept;

public:
    static constexpr std::size_t defaultGarbageCollectTargetNodeCount =
        1UL << 20; // 1M nodes or about 64MiB
    bool needGarbageCollect(
        std::size_t garbageCollectTargetNodeCount = defaultGarbageCollectTargetNodeCount) noexcept
    {
        return garbageCollectTargetNodeCount < nodeCount;
    }
    void garbageCollect(
        HashlifeNodeBase *const *const *rootPointerArrays,
        const std::size_t *rootPointerArraySizes,
        std::size_t rootPointerArrayCount,
        std::size_t garbageCollectTargetNodeCount = defaultGarbageCollectTargetNodeCount) noexcept;
    void garbageCollect(
        HashlifeNodeBase *const *rootPointers,
        std::size_t rootPointerCount,
        std::size_t garbageCollectTargetNodeCount = defaultGarbageCollectTargetNodeCount) noexcept
    {
        garbageCollect(&rootPointers, &rootPointerCount, 1, garbageCollectTargetNodeCount);
    }
    void garbageCollect(
        std::initializer_list<HashlifeNodeBase *> rootPointers,
        std::size_t garbageCollectTargetNodeCount = defaultGarbageCollectTargetNodeCount) noexcept
    {
        garbageCollect(rootPointers.begin(), rootPointers.size(), garbageCollectTargetNodeCount);
    }
    void garbageCollect(
        std::initializer_list<HashlifeNodeBase *> rootPointers0,
        std::initializer_list<HashlifeNodeBase *> rootPointers1,
        std::size_t garbageCollectTargetNodeCount = defaultGarbageCollectTargetNodeCount) noexcept
    {
        HashlifeNodeBase *const *const rootPointersArray[2] = {
            rootPointers0.begin(), rootPointers1.begin(),
        };
        const std::size_t rootPointerArraySizes[2] = {
            rootPointers0.size(), rootPointers1.size(),
        };
        garbageCollect(rootPointersArray, rootPointerArraySizes, 2, garbageCollectTargetNodeCount);
    }
};
}
}
}

#endif /* WORLD_HASHLIFE_GC_HASHTABLE_H_ */
