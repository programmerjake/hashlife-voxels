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
#include "hashlife_gc_hashtable.h"

namespace programmerjake
{
namespace voxels
{
namespace world
{
void HashlifeGarbageCollectedHashtable::unlinkAndFreeNode(HashlifeNodeBase *node) noexcept
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

HashlifeGarbageCollectedHashtable::HashlifeGarbageCollectedHashtable()
    : buckets(*new util::Array<HashlifeNodeBase *, bucketCount>()),
      canonicalEmptyNodes{},
      nodeCount(0),
      collectPendingNodeQueueHead(nullptr),
      collectPendingNodeQueueTail(nullptr)
{
    buckets.fill(nullptr);
}

HashlifeGarbageCollectedHashtable::~HashlifeGarbageCollectedHashtable()
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

void HashlifeGarbageCollectedHashtable::markNode(HashlifeNodeBase *node,
                                                 HashlifeNodeBase *&worklistHead)
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

void HashlifeGarbageCollectedHashtable::freeNodesFromCollectPendingNodeQueue(
    std::size_t &numberOfNodesLeftToCollect) noexcept
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

void HashlifeGarbageCollectedHashtable::garbageCollect(
    HashlifeNodeBase *const *const *rootPointerArrays,
    const std::size_t *rootPointerArraySizes,
    std::size_t rootPointerArrayCount,
    std::size_t garbageCollectTargetNodeCount) noexcept
{
    if(!needGarbageCollect(garbageCollectTargetNodeCount))
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
            auto nonleafNode = getAsNonleaf(node);
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
}
}
}
