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
#include <array>
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
    std::array<HashlifeNodeBase *, bucketCount> &buckets;
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
    void unlinkAndFreeNode(HashlifeNodeBase *node) noexcept
    {
        constexprAssert(node);
        constexprAssert(!node->garbageCollectState.markFlag);
        constexprAssert(!node->garbageCollectState.stateUnmarked.isInCollectPendingNodeQueue(
            collectPendingNodeQueueHead));
        HashlifeNodeBase **pBucket = &buckets[getBucketIndex(*node)];
        for(auto ppNode = pBucket; *ppNode; ppNode = &(*ppNode)->hashNext)
        {
            if(*ppNode == node)
            {
                *ppNode = node->hashNext;
                node->hashNext = nullptr;
                HashlifeNodeBase::free(node);
                return;
            }
        }
        constexprAssert(!"node not found in bucket");
    }

public:
    HashlifeGarbageCollectedHashtable()
        : buckets(*new std::array<HashlifeNodeBase *, bucketCount>()),
          canonicalEmptyNodes{},
          nodeCount(0),
          collectPendingNodeQueueHead(nullptr),
          collectPendingNodeQueueTail(nullptr)
    {
        buckets.fill(nullptr);
    }
    ~HashlifeGarbageCollectedHashtable()
    {
        for(std::size_t bucketIndex = 0; bucketIndex < bucketCount; bucketIndex++)
        {
            auto pNode = buckets[bucketIndex];
            while(pNode)
            {
                auto next = pNode->hashNext;
                delete pNode;
                pNode = next;
            }
        }
        delete &buckets;
    }
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
    void markNode(HashlifeNodeBase *node, HashlifeNodeBase *&worklistHead)
    {
        if(node && !node->garbageCollectState.markFlag)
        {
            if(node->garbageCollectState.stateUnmarked.isInCollectPendingNodeQueue(
                   collectPendingNodeQueueHead))
                node->garbageCollectState.stateUnmarked.removeFromCollectPendingNodeQueue(
                    collectPendingNodeQueueHead, collectPendingNodeQueueTail);
            node->garbageCollectState.markFlag = true;
            destruct(node->garbageCollectState.stateUnmarked);
            construct(node->garbageCollectState.stateMarked);
            node->garbageCollectState.stateMarked.worklistNext = worklistHead;
            worklistHead = node;
        }
    }
    void freeNodesFromCollectPendingNodeQueue(std::size_t &numberOfNodesLeftToCollect) noexcept
    {
        while(numberOfNodesLeftToCollect > 0 && collectPendingNodeQueueHead)
        {
            auto node = collectPendingNodeQueueHead;
            node->garbageCollectState.stateUnmarked.removeFromCollectPendingNodeQueue(
                collectPendingNodeQueueHead, collectPendingNodeQueueTail);
            unlinkAndFreeNode(node);
            numberOfNodesLeftToCollect--;
        }
    }

public:
    static constexpr std::size_t defaultGarbageCollectTargetNodeCount =
        1UL << 20; // 1M nodes or about 64MiB
    void garbageCollect(
        HashlifeNodeBase *const *const *rootPointerArrays,
        const std::size_t *rootPointerArraySizes,
        std::size_t rootPointerArrayCount,
        std::size_t garbageCollectTargetNodeCount = defaultGarbageCollectTargetNodeCount) noexcept
    {
        if(garbageCollectTargetNodeCount >= nodeCount)
            return;
        std::size_t numberOfNodesLeftToCollect = nodeCount - garbageCollectTargetNodeCount;
        freeNodesFromCollectPendingNodeQueue(numberOfNodesLeftToCollect);
        if(numberOfNodesLeftToCollect == 0)
            return;
        HashlifeNodeBase *worklistHead = nullptr;
        // mark roots
        for(std::size_t rootPointerArrayIndex = 0; rootPointerArrayIndex < rootPointerArrayCount;
            rootPointerArrayIndex++)
        {
            auto rootPointers = rootPointerArrays[rootPointerArrayIndex];
            auto rootPointersSize = rootPointerArraySizes[rootPointerArrayIndex];
            for(std::size_t rootPointerIndex = 0; rootPointerIndex < rootPointersSize;
                rootPointerIndex++)
                markNode(rootPointers[rootPointerIndex], worklistHead);
        }
        for(auto root : canonicalEmptyNodes)
        {
            markNode(root, worklistHead);
        }
        // mark derived pointers
        while(worklistHead)
        {
            auto node = worklistHead;
            worklistHead = node->garbageCollectState.stateMarked.worklistNext;
            node->garbageCollectState.stateMarked.worklistNext = nullptr;
            if(node->isLeaf())
            {
                // no derived pointers
            }
            else
            {
                auto nonleafNode = static_cast<HashlifeNonleafNode *>(node);
                for(util::Vector3I32 position(0); position.x < HashlifeNodeBase::levelSize;
                    position.x++)
                {
                    for(position.y = 0; position.y < HashlifeNodeBase::levelSize; position.y++)
                    {
                        for(position.z = 0; position.z < HashlifeNodeBase::levelSize; position.z++)
                        {
                            markNode(nonleafNode->getChildNode(position), worklistHead);
                        }
                    }
                }
                markNode(nonleafNode->futureState.node, worklistHead);
            }
        }
        // all the reachable nodes are now marked
        // walk heap adding unreachable nodes to collectPendingNodeQueue and clearing marks
        for(std::size_t bucketIndex = 0; bucketIndex < bucketCount; bucketIndex++)
        {
            for(auto node = buckets[bucketIndex]; node; node = node->hashNext)
            {
                if(node->garbageCollectState.markFlag)
                {
                    constexprAssert(node->garbageCollectState.stateMarked.worklistNext == nullptr);
                    node->garbageCollectState.markFlag = false;
                    destruct(node->garbageCollectState.stateMarked);
                    construct(node->garbageCollectState.stateUnmarked);
                }
                else if(!node->garbageCollectState.stateUnmarked.isInCollectPendingNodeQueue(
                            collectPendingNodeQueueHead))
                {
                    node->garbageCollectState.stateUnmarked.addToCollectPendingNodeQueue(
                        node, collectPendingNodeQueueHead, collectPendingNodeQueueTail);
                }
            }
        }
        // collect more nodes
        freeNodesFromCollectPendingNodeQueue(numberOfNodesLeftToCollect);
    }
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
