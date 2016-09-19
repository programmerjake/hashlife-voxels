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
#include <list>

namespace programmerjake
{
namespace voxels
{
namespace world
{
/** @note
 * this class holds a non-atomic reference to each hashlife node contained in it
 */
class HashlifeGarbageCollectedHashtable final
{
    HashlifeGarbageCollectedHashtable(const HashlifeGarbageCollectedHashtable &) = delete;
    HashlifeGarbageCollectedHashtable &operator=(const HashlifeGarbageCollectedHashtable &) =
        delete;

private:
    static constexpr std::size_t bucketCount = static_cast<std::size_t>(1) << 20;
    const HashlifeNodeBase **buckets;
    std::size_t nodeCount;
    HashlifeNodeReference<const HashlifeNodeBase, false>
        canonicalEmptyNodes[HashlifeNodeBase::maxLevel + 1];

private:
    static HashlifeNodeReference<const HashlifeLeafNode, false> makeNode(
        const HashlifeLeafNode::BlocksArray &blocks)
    {
        return HashlifeNodeReference<const HashlifeLeafNode, false>(new HashlifeLeafNode(blocks));
    }
    static HashlifeNodeReference<const HashlifeNonleafNode, false> makeNode(
        HashlifeNonleafNode::ChildNodesArray childNodes)
    {
        return HashlifeNodeReference<const HashlifeNonleafNode, false>(
            new HashlifeNonleafNode(std::move(childNodes)));
    }
    static std::size_t hashNode(const HashlifeLeafNode::BlocksArray &blocks)
    {
        return HashlifeLeafNode::hashNode(blocks);
    }
    static std::size_t hashNode(const HashlifeNonleafNode::ChildNodesArray &childNodes)
    {
        return HashlifeNonleafNode::hashNode(childNodes);
    }
    static bool equalsNode(const HashlifeNodeBase *node,
                           const HashlifeNonleafNode::ChildNodesArray &childNodes)
    {
        return HashlifeNonleafNode::equalsNode(node, childNodes);
    }
    static bool equalsNode(const HashlifeNodeBase *node,
                           const HashlifeLeafNode::BlocksArray &blocks)
    {
        return HashlifeLeafNode::equalsNode(node, blocks);
    }
    static bool isLeaf(const HashlifeNonleafNode::ChildNodesArray &) noexcept
    {
        return false;
    }
    template <typename NodeType>
    HashlifeNodeReference<const HashlifeNodeBase, false> findOrAddNodeImplementation(
        NodeType &&nodeIn)
    {
        std::size_t index = hashNode(nodeIn) % bucketCount;
        for(auto **ppNode = &buckets[index]; *ppNode != nullptr; ppNode = &(*ppNode)->hashNext)
        {
            auto *pNode = *ppNode;
            if(equalsNode(pNode, nodeIn))
            {
                *ppNode = pNode->hashNext; // move node to front of linked list
                pNode->hashNext = buckets[index];
                buckets[index] = pNode;
                return pNode->referenceFromThis<false>();
            }
        }
        HashlifeNodeReference<const HashlifeNodeBase, false> newNode =
            makeNode(std::forward<NodeType>(nodeIn));
        nodeCount++;
        HashlifeNodeReference<const HashlifeNodeBase, false> retval = newNode;
        newNode->hashNext = buckets[index];
        buckets[index] = newNode.release();
        return retval;
    }

public:
    HashlifeGarbageCollectedHashtable()
        : buckets(new const HashlifeNodeBase *[bucketCount]()), nodeCount(0), canonicalEmptyNodes{}
    {
    }
    ~HashlifeGarbageCollectedHashtable()
    {
        for(std::size_t i = 0; i < bucketCount; i++)
        {
            HashlifeNodeReference<const HashlifeNodeBase, false> node(buckets[i]);
            while(node != nullptr)
            {
                HashlifeNodeReference<const HashlifeNodeBase, false> nextNode(node->hashNext);
                node->hashNext = nullptr;
                node = nextNode;
            }
        }
        delete []buckets;
    }
    HashlifeNodeReference<const HashlifeNodeBase, false> findOrAddNode(
        HashlifeNonleafNode::ChildNodesArray &&childNodes)
    {
        return findOrAddNodeImplementation(std::move(childNodes));
    }
    HashlifeNodeReference<const HashlifeNodeBase, false> findOrAddNode(
        const HashlifeNonleafNode::ChildNodesArray &childNodes)
    {
        return findOrAddNodeImplementation(childNodes);
    }
    HashlifeNodeReference<const HashlifeNodeBase, false> findOrAddNode(
        const HashlifeLeafNode::BlocksArray &blocks)
    {
        return findOrAddNodeImplementation(blocks);
    }
    const HashlifeNodeReference<const HashlifeNodeBase, false> &getCanonicalEmptyNode(
        HashlifeNodeBase::LevelType level)
    {
        constexprAssert(level <= HashlifeNodeBase::maxLevel);
        if(!canonicalEmptyNodes[level])
        {
            if(HashlifeNodeBase::isLeaf(level))
            {
                canonicalEmptyNodes[level] =
                    findOrAddNode(HashlifeLeafNode::BlocksArray{block::Block(),
                                                                block::Block(),
                                                                block::Block(),
                                                                block::Block(),
                                                                block::Block(),
                                                                block::Block(),
                                                                block::Block(),
                                                                block::Block()});
            }
            else
            {
                auto previousCanonicalEmptyNode = getCanonicalEmptyNode(level - 1);
                canonicalEmptyNodes[level] =
                    findOrAddNode(HashlifeNonleafNode::ChildNodesArray{previousCanonicalEmptyNode,
                                                                       previousCanonicalEmptyNode,
                                                                       previousCanonicalEmptyNode,
                                                                       previousCanonicalEmptyNode,
                                                                       previousCanonicalEmptyNode,
                                                                       previousCanonicalEmptyNode,
                                                                       previousCanonicalEmptyNode,
                                                                       previousCanonicalEmptyNode});
            }
        }
        return canonicalEmptyNodes[level];
    }
    static constexpr std::size_t defaultGarbageCollectTargetNodeCount =
        1UL << 20; // 1M nodes or about 64MiB
    bool needGarbageCollect(
        std::size_t garbageCollectTargetNodeCount = defaultGarbageCollectTargetNodeCount) noexcept
    {
        return garbageCollectTargetNodeCount < nodeCount;
    }
    void garbageCollect(
        std::size_t garbageCollectTargetNodeCount = defaultGarbageCollectTargetNodeCount) noexcept;
};
}
}
}

#endif /* WORLD_HASHLIFE_GC_HASHTABLE_H_ */
