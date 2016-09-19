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
void HashlifeGarbageCollectedHashtable::garbageCollect(
    std::size_t garbageCollectTargetNodeCount) noexcept
{
    if(!needGarbageCollect(garbageCollectTargetNodeCount))
        return;
    std::size_t numberOfNodesLeftToCollect = nodeCount - garbageCollectTargetNodeCount;
    for(std::size_t collectedCount = 1; collectedCount > 0;)
    {
        collectedCount = 0;
        for(std::size_t bucketIndex = 0; bucketIndex < bucketCount; bucketIndex++)
        {
            for(auto **ppNode = &buckets[bucketIndex]; *ppNode != nullptr;
                ppNode = &(*ppNode)->hashNext)
            {
                HashlifeNodeReference<const HashlifeNodeBase, false> node(*ppNode);
                if(node.useCount() > 1
                   || node->referenceCounts.atomicReferenceCount.load(std::memory_order_relaxed)
                          > 1)
                {
                    node.release();
                    continue;
                }
                *ppNode = node->hashNext;
                node->hashNext = nullptr;
                collectedCount++;
                if(*ppNode == nullptr)
                    break;
            }
        }
        nodeCount -= collectedCount;
        if(collectedCount >= numberOfNodesLeftToCollect)
            return;
        numberOfNodesLeftToCollect -= collectedCount;
    }
}
}
}
}
