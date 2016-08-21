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

#ifndef WORLD_HASHLIFE_WORLD_H_
#define WORLD_HASHLIFE_WORLD_H_

#include "hashlife_gc_hashtable.h"
#include "hashlife_node.h"
#include "../block/block.h"
#include "../block/block_descriptor.h"
#include <memory>
#include <list>

namespace programmerjake
{
namespace voxels
{
namespace world
{
class HashlifeWorld final : public std::enable_shared_from_this<HashlifeWorld>
{
    HashlifeWorld(const HashlifeWorld &) = delete;
    HashlifeWorld &operator=(const HashlifeWorld &) = delete;

private:
    struct PrivateAccessTag
    {
        friend HashlifeWorld;

    private:
        PrivateAccessTag() = default;
    };

private:
    static block::Block getBlock(HashlifeNodeBase *rootNode, util::Vector3I32 position) noexcept
    {
        if(rootNode->isPositionInside(position))
            return rootNode->get(position);
        return block::Block();
    }

public:
    class Snapshot final
    {
        friend class HashlifeWorld;
        Snapshot(const Snapshot &) = delete;
        Snapshot &operator=(const Snapshot &) = delete;

    private:
        HashlifeNodeBase *rootNode;
        std::shared_ptr<const HashlifeWorld> world;
        Snapshot(HashlifeNodeBase *rootNode,
                 std::shared_ptr<const HashlifeWorld> world,
                 PrivateAccessTag)
            : rootNode(rootNode), world(std::move(world))
        {
        }

    public:
        ~Snapshot() = default;
        block::Block get(util::Vector3I32 position) const noexcept
        {
            return getBlock(rootNode, position);
        }
    };

private:
    HashlifeGarbageCollectedHashtable garbageCollectedHashtable;
    mutable std::list<std::weak_ptr<Snapshot>> snapshots;
    HashlifeNodeBase *rootNode;

private:
    explicit HashlifeWorld(PrivateAccessTag)
        : garbageCollectedHashtable(),
          snapshots(),
          rootNode(garbageCollectedHashtable.getCanonicalEmptyNode(1))
    {
    }

public:
    void collectGarbage(std::size_t garbageCollectTargetNodeCount =
                            HashlifeGarbageCollectedHashtable::defaultGarbageCollectTargetNodeCount)
    {
        std::vector<HashlifeNodeBase *> roots;
        roots.reserve(1 + snapshots.size());
        roots.push_back(rootNode);
        for(auto iter = snapshots.begin(); iter != snapshots.end();)
        {
            auto snapshot = iter->lock();
            if(snapshot)
            {
                roots.push_back(snapshot->rootNode);
                ++iter;
            }
            else
            {
                iter = snapshots.erase(iter);
            }
        }
        garbageCollectedHashtable.garbageCollect(
            roots.data(), roots.size(), garbageCollectTargetNodeCount);
    }
    std::shared_ptr<const Snapshot> makeSnapshot() const
    {
        auto retval = std::make_shared<Snapshot>(rootNode, shared_from_this(), PrivateAccessTag());
        snapshots.push_back(retval);
        return retval;
    }
    static std::shared_ptr<HashlifeWorld> make()
    {
        return std::make_shared<HashlifeWorld>(PrivateAccessTag());
    }
    block::Block get(util::Vector3I32 position) const noexcept
    {
        return getBlock(rootNode, position);
    }

private:
    void expandRoot()
    {
        constexprAssert(!rootNode->isLeaf());
        auto emptyNode = garbageCollectedHashtable.getCanonicalEmptyNode(rootNode->level - 1);
        HashlifeNonleafNode::ChildNodesArray newRootNode;
        for(std::size_t x = 0; x < newRootNode.size(); x++)
        {
            for(std::size_t y = 0; y < newRootNode[x].size(); y++)
            {
                for(std::size_t z = 0; z < newRootNode[x][y].size(); z++)
                {
                    HashlifeNonleafNode::ChildNodesArray newChildNode{emptyNode,
                                                                      emptyNode,
                                                                      emptyNode,
                                                                      emptyNode,
                                                                      emptyNode,
                                                                      emptyNode,
                                                                      emptyNode,
                                                                      emptyNode};
                    static_assert(HashlifeNonleafNode::ChildNodesArray().size() == 2, "");
                    static_assert(HashlifeNonleafNode::ChildNodesArray()[0].size() == 2, "");
                    static_assert(HashlifeNonleafNode::ChildNodesArray()[0][0].size() == 2, "");
                    newChildNode[2 - x - 1][2 - y - 1][2 - z - 1] =
                        static_cast<HashlifeNonleafNode *>(rootNode)->childNodes[x][y][z];
                    newRootNode[x][y][z] =
                        garbageCollectedHashtable.findOrAddNode(HashlifeNonleafNode(newChildNode));
                }
            }
        }
        rootNode = garbageCollectedHashtable.findOrAddNode(HashlifeNonleafNode(newRootNode));
    }
    HashlifeNonleafNode::FutureState &getFilledFutureState(
        HashlifeNodeBase *nodeIn, const block::BlockStepGlobalState &stepGlobalState)
    {
        constexprAssert(!nodeIn->isLeaf());
        auto node = static_cast<HashlifeNonleafNode *>(nodeIn);
        if(node->futureState.node && node->futureState.globalState == stepGlobalState)
            return node->futureState;
        node->futureState = HashlifeNonleafNode::FutureState(
            nullptr, stepGlobalState, block::BlockStepExtraActions());
        if(node->level == 1)
        {
            HashlifeLeafNode::BlocksArray futureNode;
            for(std::size_t x = 0; x < futureNode.size(); x++)
            {
                for(std::size_t y = 0; y < futureNode[x].size(); y++)
                {
                    for(std::size_t z = 0; z < futureNode[x][y].size(); z++)
                    {
                        block::BlockStepInput blockStepInput;
                        static_assert(HashlifeNonleafNode::ChildNodesArray().size() == 2, "");
                        static_assert(HashlifeNonleafNode::ChildNodesArray()[0].size() == 2, "");
                        static_assert(HashlifeNonleafNode::ChildNodesArray()[0][0].size() == 2, "");
                        util::Vector3I32 blockStepInputOrigin(x - 2, y - 2, z - 2);
                        auto blockStepInputCenter = blockStepInputOrigin + util::Vector3I32(1);
                        for(std::size_t x2 = 0; x2 < blockStepInput.blocks.size(); x2++)
                        {
                            for(std::size_t y2 = 0; y2 < blockStepInput.blocks[x2].size(); y2++)
                            {
                                for(std::size_t z2 = 0; z2 < blockStepInput.blocks[x2][y2].size();
                                    z2++)
                                {
                                    blockStepInput.blocks[x2][y2][z2] = node->get(
                                        util::Vector3I32(x2, y2, z2) + blockStepInputOrigin);
                                }
                            }
                        }
                        auto stepResult =
                            block::BlockDescriptor::step(blockStepInput, stepGlobalState);
                        futureNode[x][y][z] = stepResult.block;
                        node->futureState.actions +=
                            std::move(stepResult.extraActions).addOffset(blockStepInputCenter);
                    }
                }
            }
            node->futureState.node =
                garbageCollectedHashtable.findOrAddNode(HashlifeLeafNode(futureNode));
        }
        else if(HashlifeNonleafNode::FutureState::getStepSizeInGenerations(node->level - 1)
                == block::BlockStepGlobalState::stepSizeInGenerations)
        {
#error finish
        }
        else
        {
#error finish
        }
        return node->futureState;
    }

public:
    util::Optional<std::list<block::BlockStepExtraAction>> stepAndCollectGarbage(
        const block::BlockStepGlobalState &stepGlobalState,
        std::size_t garbageCollectTargetNodeCount =
            HashlifeGarbageCollectedHashtable::defaultGarbageCollectTargetNodeCount)
    {
        collectGarbage(garbageCollectTargetNodeCount);
        return step(stepGlobalState);
    }
    util::Optional<std::list<block::BlockStepExtraAction>> step(
        const block::BlockStepGlobalState &stepGlobalState)
    {
        do
        {
            expandRoot();
        } while(HashlifeNonleafNode::FutureState::getStepSizeInGenerations(rootNode->level)
                < block::BlockStepGlobalState::stepSizeInGenerations);
        auto futureState = getFilledFutureState(rootNode, stepGlobalState);
        constexprAssert(futureState.node != nullptr);
        constexprAssert(futureState.globalState == stepGlobalState);
        rootNode = futureState.node;
        return futureState.actions;
    }
};
}
}
}

#endif /* WORLD_HASHLIFE_WORLD_H_ */
