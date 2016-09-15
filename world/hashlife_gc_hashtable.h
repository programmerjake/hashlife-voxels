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
#include <unordered_map>

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
    struct NodeHasher
    {
        std::size_t operator()(const HashlifeNodeBase *v) const
        {
            return v->hash();
        }
    };
    struct NodeEquals
    {
        bool operator()(const HashlifeNodeBase *a, const HashlifeNodeBase *b) const
        {
            return *a == *b;
        }
    };

private:
    std::shared_ptr<const HashlifeNodeBase> canonicalEmptyNodes[HashlifeNodeBase::maxLevel + 1];
    std::unordered_map<const HashlifeNodeBase *,
                       std::shared_ptr<const HashlifeNodeBase>,
                       NodeHasher,
                       NodeEquals> nodes;
    template <typename NodeType>
    const std::shared_ptr<const HashlifeNodeBase> &findOrAddNodeImplementation(NodeType &&nodeIn)
    {
        auto iter = nodes.find(&nodeIn);
        if(iter == nodes.end())
        {
            auto newNode = std::forward<NodeType>(nodeIn).duplicate();
            iter = std::get<0>(nodes.emplace(newNode.get(), std::move(newNode)));
        }
        return std::get<1>(*iter);
    }

public:
    HashlifeGarbageCollectedHashtable() = default;
    const std::shared_ptr<const HashlifeNodeBase> &findOrAddNode(const HashlifeNodeBase &nodeIn)
    {
        return findOrAddNodeImplementation(nodeIn);
    }
    const std::shared_ptr<const HashlifeNodeBase> &findOrAddNode(HashlifeNodeBase &&nodeIn)
    {
        return findOrAddNodeImplementation(std::move(nodeIn));
    }
    const std::shared_ptr<const HashlifeNodeBase> &getCanonicalEmptyNode(
        HashlifeNodeBase::LevelType level)
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
    static constexpr std::size_t defaultGarbageCollectTargetNodeCount =
        1UL << 20; // 1M nodes or about 64MiB
    bool needGarbageCollect(
        std::size_t garbageCollectTargetNodeCount = defaultGarbageCollectTargetNodeCount) noexcept
    {
        return garbageCollectTargetNodeCount < nodes.size();
    }
    void garbageCollect(
        std::size_t garbageCollectTargetNodeCount = defaultGarbageCollectTargetNodeCount) noexcept;
};
}
}
}

#endif /* WORLD_HASHLIFE_GC_HASHTABLE_H_ */
