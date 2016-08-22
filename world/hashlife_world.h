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

    public:
        Snapshot(HashlifeNodeBase *rootNode,
                 std::shared_ptr<const HashlifeWorld> world,
                 PrivateAccessTag)
            : rootNode(rootNode), world(std::move(world))
        {
        }
        ~Snapshot() = default;
        block::Block get(util::Vector3I32 position) const noexcept
        {
            return getBlock(rootNode, position);
        }
        util::Vector3I32 minPosition() const noexcept
        {
            return util::Vector3I32(-rootNode->getHalfSize());
        }
        util::Vector3I32 endPosition() const noexcept
        {
            return util::Vector3I32(rootNode->getHalfSize());
        }
        util::Vector3I32 maxPosition() const noexcept
        {
            return util::Vector3I32(rootNode->getHalfSize() - 1);
        }
    };

private:
    HashlifeGarbageCollectedHashtable garbageCollectedHashtable;
    mutable std::list<std::weak_ptr<Snapshot>> snapshots;
    HashlifeNodeBase *rootNode;

public:
    explicit HashlifeWorld(PrivateAccessTag)
        : garbageCollectedHashtable(),
          snapshots(),
          rootNode(garbageCollectedHashtable.getCanonicalEmptyNode(1))
    {
    }
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
                    static_assert(HashlifeNodeBase::levelSize == 2, "");
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
        node->futureState = HashlifeNonleafNode::FutureState(stepGlobalState);
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
                        static_assert(HashlifeNodeBase::levelSize == 2, "");
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
                        node->futureState.actions[x][y][z] +=
                            std::move(stepResult.extraActions).addOffset(blockStepInputCenter);
                    }
                }
            }
            node->futureState.node =
                garbageCollectedHashtable.findOrAddNode(HashlifeLeafNode(futureNode));
        }
        else
        {
            static_assert(HashlifeNodeBase::levelSize == 2, "");
            static constexpr std::int32_t intermediateSize = HashlifeNodeBase::levelSize * 2 - 1;
            std::array<std::array<std::array<HashlifeNodeBase *, intermediateSize>,
                                  intermediateSize>,
                       intermediateSize> intermediate;
            for(util::Vector3I32 chunkPos(0); chunkPos.x < intermediateSize; chunkPos.x++)
            {
                for(chunkPos.y = 0; chunkPos.y < intermediateSize; chunkPos.y++)
                {
                    for(chunkPos.z = 0; chunkPos.z < intermediateSize; chunkPos.z++)
                    {
                        HashlifeNonleafNode::ChildNodesArray input;
                        for(util::Vector3I32 position(0); position.x < HashlifeNodeBase::levelSize;
                            position.x++)
                        {
                            for(position.y = 0; position.y < HashlifeNodeBase::levelSize;
                                position.y++)
                            {
                                for(position.z = 0; position.z < HashlifeNodeBase::levelSize;
                                    position.z++)
                                {
                                    auto inputPosition = chunkPos + position;
                                    auto index1 = inputPosition
                                                  / util::Vector3I32(HashlifeNodeBase::levelSize);
                                    auto index2 = inputPosition
                                                  % util::Vector3I32(HashlifeNodeBase::levelSize);
                                    auto childNode = node->childNodes[index1.x][index1.y][index1.z];
                                    constexprAssert(!childNode->isLeaf());
                                    input[position.x][position.y][position.z] =
                                        static_cast<HashlifeNonleafNode *>(childNode)
                                            ->childNodes[index2.x][index2.y][index2.z];
                                }
                            }
                        }
                        auto &result = getFilledFutureState(
                            garbageCollectedHashtable.findOrAddNode(HashlifeNonleafNode(input)),
                            stepGlobalState);
                        for(util::Vector3I32 position(0); position.x < HashlifeNodeBase::levelSize;
                            position.x++)
                        {
                            for(position.y = 0; position.y < HashlifeNodeBase::levelSize;
                                position.y++)
                            {
                                for(position.z = 0; position.z < HashlifeNodeBase::levelSize;
                                    position.z++)
                                {
                                    static_assert(HashlifeNodeBase::levelSize == 2, "");
                                    auto offsetInEighths =
                                        (chunkPos - util::Vector3I32(1))
                                        * util::Vector3I32(HashlifeNodeBase::levelSize);
                                    auto outputPosition =
                                        (position + offsetInEighths + util::Vector3I32(1))
                                        / util::Vector3I32(HashlifeNodeBase::levelSize);
                                    if(outputPosition.x < 0
                                       || outputPosition.x >= HashlifeNodeBase::levelSize
                                       || outputPosition.y < 0
                                       || outputPosition.y >= HashlifeNodeBase::levelSize
                                       || outputPosition.z < 0
                                       || outputPosition.z >= HashlifeNodeBase::levelSize)
                                        continue;
                                    node->futureState.actions[outputPosition.x][outputPosition.y]
                                                             [outputPosition.z] +=
                                        block::BlockStepExtraActions(
                                            result.actions[position.x][position.y][position.z])
                                            .addOffset(offsetInEighths
                                                       * util::Vector3I32(node->getEighthSize()));
                                }
                            }
                        }
                        intermediate[chunkPos.x][chunkPos.y][chunkPos.z] = result.node;
                    }
                }
            }
            HashlifeNonleafNode::ChildNodesArray output;
            if(HashlifeNonleafNode::FutureState::getStepSizeInGenerations(node->level - 1)
               == block::BlockStepGlobalState::stepSizeInGenerations)
            {
                for(util::Vector3I32 chunkPos(0); chunkPos.x < HashlifeNodeBase::levelSize;
                    chunkPos.x++)
                {
                    for(chunkPos.y = 0; chunkPos.y < HashlifeNodeBase::levelSize; chunkPos.y++)
                    {
                        for(chunkPos.z = 0; chunkPos.z < HashlifeNodeBase::levelSize; chunkPos.z++)
                        {
                            if(node->getEighthSize() == 1)
                            {
                                HashlifeLeafNode::BlocksArray blocks;
                                for(util::Vector3I32 position(0);
                                    position.x < HashlifeNodeBase::levelSize;
                                    position.x++)
                                {
                                    for(position.y = 0; position.y < HashlifeNodeBase::levelSize;
                                        position.y++)
                                    {
                                        for(position.z = 0;
                                            position.z < HashlifeNodeBase::levelSize;
                                            position.z++)
                                        {
                                            auto inputPosition =
                                                chunkPos
                                                    * util::Vector3I32(HashlifeNodeBase::levelSize)
                                                + position + util::Vector3I32(1);
                                            auto index1 =
                                                inputPosition
                                                / util::Vector3I32(HashlifeNodeBase::levelSize);
                                            auto index2 =
                                                inputPosition
                                                % util::Vector3I32(HashlifeNodeBase::levelSize);
                                            auto childNode =
                                                intermediate[index1.x][index1.y][index1.z];
                                            constexprAssert(childNode->isLeaf());
                                            blocks[position.x][position.y][position.z] =
                                                static_cast<HashlifeLeafNode *>(childNode)
                                                    ->blocks[index2.x][index2.y][index2.z];
                                        }
                                    }
                                }
                                output[chunkPos.x][chunkPos.y][chunkPos.z] =
                                    garbageCollectedHashtable.findOrAddNode(
                                        HashlifeLeafNode(blocks));
                            }
                            else
                            {
                                HashlifeNonleafNode::ChildNodesArray childNodes;
                                for(util::Vector3I32 position(0);
                                    position.x < HashlifeNodeBase::levelSize;
                                    position.x++)
                                {
                                    for(position.y = 0; position.y < HashlifeNodeBase::levelSize;
                                        position.y++)
                                    {
                                        for(position.z = 0;
                                            position.z < HashlifeNodeBase::levelSize;
                                            position.z++)
                                        {
                                            auto inputPosition =
                                                chunkPos
                                                    * util::Vector3I32(HashlifeNodeBase::levelSize)
                                                + position + util::Vector3I32(1);
                                            auto index1 =
                                                inputPosition
                                                / util::Vector3I32(HashlifeNodeBase::levelSize);
                                            auto index2 =
                                                inputPosition
                                                % util::Vector3I32(HashlifeNodeBase::levelSize);
                                            auto childNode =
                                                intermediate[index1.x][index1.y][index1.z];
                                            constexprAssert(!childNode->isLeaf());
                                            childNodes[position.x][position.y][position.z] =
                                                static_cast<HashlifeNonleafNode *>(childNode)
                                                    ->childNodes[index2.x][index2.y][index2.z];
                                        }
                                    }
                                }
                                output[chunkPos.x][chunkPos.y][chunkPos.z] =
                                    garbageCollectedHashtable.findOrAddNode(
                                        HashlifeNonleafNode(childNodes));
                            }
                        }
                    }
                }
            }
            else
            {
                for(util::Vector3I32 chunkPos(0); chunkPos.x < HashlifeNodeBase::levelSize;
                    chunkPos.x++)
                {
                    for(chunkPos.y = 0; chunkPos.y < HashlifeNodeBase::levelSize; chunkPos.y++)
                    {
                        for(chunkPos.z = 0; chunkPos.z < HashlifeNodeBase::levelSize; chunkPos.z++)
                        {
                            HashlifeNonleafNode::ChildNodesArray input;
                            for(util::Vector3I32 position(0);
                                position.x < HashlifeNodeBase::levelSize;
                                position.x++)
                            {
                                for(position.y = 0; position.y < HashlifeNodeBase::levelSize;
                                    position.y++)
                                {
                                    for(position.z = 0; position.z < HashlifeNodeBase::levelSize;
                                        position.z++)
                                    {
                                        auto inputPosition = chunkPos + position;
                                        input[position.x][position.y][position.z] =
                                            intermediate[inputPosition.x][inputPosition
                                                                              .y][inputPosition.z];
                                    }
                                }
                            }
                            auto &result = getFilledFutureState(
                                garbageCollectedHashtable.findOrAddNode(HashlifeNonleafNode(input)),
                                stepGlobalState);
                            for(util::Vector3I32 position(0);
                                position.x < HashlifeNodeBase::levelSize;
                                position.x++)
                            {
                                for(position.y = 0; position.y < HashlifeNodeBase::levelSize;
                                    position.y++)
                                {
                                    for(position.z = 0; position.z < HashlifeNodeBase::levelSize;
                                        position.z++)
                                    {
                                        static_assert(HashlifeNodeBase::levelSize == 2, "");
                                        auto offsetInEighths =
                                            chunkPos * util::Vector3I32(HashlifeNodeBase::levelSize)
                                            - util::Vector3I32(1);
                                        node->futureState
                                            .actions[chunkPos.x][chunkPos.y][chunkPos.z] +=
                                            block::BlockStepExtraActions(
                                                result.actions[position.x][position.y][position.z])
                                                .addOffset(
                                                    offsetInEighths
                                                    * util::Vector3I32(node->getEighthSize()));
                                    }
                                }
                            }
                            output[chunkPos.x][chunkPos.y][chunkPos.z] = result.node;
                        }
                    }
                }
            }
            node->futureState.node =
                garbageCollectedHashtable.findOrAddNode(HashlifeNonleafNode(output));
        }
        return node->futureState;
    }

public:
    block::BlockStepExtraActions stepAndCollectGarbage(
        const block::BlockStepGlobalState &stepGlobalState,
        std::size_t garbageCollectTargetNodeCount =
            HashlifeGarbageCollectedHashtable::defaultGarbageCollectTargetNodeCount)
    {
        collectGarbage(garbageCollectTargetNodeCount);
        return step(stepGlobalState);
    }
    block::BlockStepExtraActions step(const block::BlockStepGlobalState &stepGlobalState)
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
        block::BlockStepExtraActions actions;
        for(auto &i : futureState.actions)
        {
            for(auto &j : i)
            {
                for(auto &v : j)
                {
                    actions += v;
                }
            }
        }
        return actions;
    }

private:
    template <typename BlocksArray>
    HashlifeNodeBase *setBlocks(HashlifeNodeBase *node,
                                BlocksArray &&blocksArray,
                                util::Vector3I32 targetPosition,
                                util::Vector3I32 sourcePosition,
                                util::Vector3I32 size)
    {
        constexprAssert(size.min() >= 0);
        constexprAssert(node->isPositionInside(targetPosition));
        if(size.min() == 0)
            return node;
        constexprAssert(node->isPositionInside(targetPosition + size - util::Vector3I32(1)));
        if(node->isLeaf())
        {
            HashlifeLeafNode::BlocksArray blocks;
            for(util::Vector3I32 position(0); position.x < HashlifeNodeBase::levelSize;
                position.x++)
            {
                for(position.y = 0; position.y < HashlifeNodeBase::levelSize; position.y++)
                {
                    for(position.z = 0; position.z < HashlifeNodeBase::levelSize; position.z++)
                    {
                        static_assert(HashlifeNodeBase::levelSize % 2 == 0, "");
                        auto inputPosition =
                            position - util::Vector3I32(HashlifeNodeBase::levelSize / 2);
                        if((inputPosition - targetPosition).min() < 0
                           || (targetPosition + size - inputPosition).max() < 0)
                        {
                            blocks[position.x][position.y][position.z] =
                                static_cast<HashlifeLeafNode *>(node)
                                    ->blocks[position.x][position.y][position.z];
                        }
                        else
                        {
                            const auto blocksArrayPosition =
                                inputPosition + sourcePosition - targetPosition;
                            blocks[position.x][position.y][position.z] = std::forward<BlocksArray>(
                                blocksArray)[blocksArrayPosition.x][blocksArrayPosition
                                                                        .y][blocksArrayPosition.z];
                        }
                    }
                }
            }
            return garbageCollectedHashtable.findOrAddNode(HashlifeLeafNode(blocks));
        }
        else
        {
            HashlifeNonleafNode::ChildNodesArray childNodes;
            for(util::Vector3I32 position(0); position.x < HashlifeNodeBase::levelSize;
                position.x++)
            {
                for(position.y = 0; position.y < HashlifeNodeBase::levelSize; position.y++)
                {
                    for(position.z = 0; position.z < HashlifeNodeBase::levelSize; position.z++)
                    {
                        static_assert(HashlifeNodeBase::levelSize % 2 == 0, "");
                        auto minInputPosition =
                            (position - util::Vector3I32(HashlifeNodeBase::levelSize / 2))
                            * util::Vector3I32(node->getHalfSize());
                        auto offset = -util::Vector3I32(node->getQuarterSize()) - minInputPosition;
                        auto endInputPosition =
                            minInputPosition + util::Vector3I32(node->getHalfSize());
                        minInputPosition = max(minInputPosition, targetPosition);
                        endInputPosition = min(endInputPosition, targetPosition + size);
                        if((endInputPosition - minInputPosition).min() > 0)
                            childNodes[position.x][position.y][position.z] =
                                setBlocks(static_cast<HashlifeNonleafNode *>(node)
                                              ->childNodes[position.x][position.y][position.z],
                                          std::forward<BlocksArray>(blocksArray),
                                          minInputPosition + offset,
                                          sourcePosition - targetPosition + minInputPosition,
                                          endInputPosition - minInputPosition);
                        else
                            childNodes[position.x][position.y][position.z] =
                                static_cast<HashlifeNonleafNode *>(node)
                                    ->childNodes[position.x][position.y][position.z];
                    }
                }
            }
            return garbageCollectedHashtable.findOrAddNode(HashlifeNonleafNode(childNodes));
        }
    }

public:
    template <typename BlocksArray>
    void setBlocks(BlocksArray &&blocksArray,
                   util::Vector3I32 targetPosition,
                   util::Vector3I32 sourcePosition,
                   util::Vector3I32 size)
    {
        constexprAssert(size.min() >= 0);
        if(size.min() <= 0)
            return;
        while(!rootNode->isPositionInside(targetPosition)
              || !rootNode->isPositionInside(targetPosition + size - util::Vector3I32(1)))
            expandRoot();
        rootNode = setBlocks(
            rootNode, std::forward<BlocksArray>(blocksArray), targetPosition, sourcePosition, size);
    }
    void setBlock(block::Block block, util::Vector3I32 position)
    {
        std::array<std::array<std::array<block::Block, 1>, 1>, 1> blocks;
        blocks[0][0][0] = block;
        setBlocks(blocks, position, util::Vector3I32(0), util::Vector3I32(1));
    }
    util::Vector3I32 minPosition() const noexcept
    {
        return util::Vector3I32(-rootNode->getHalfSize());
    }
    util::Vector3I32 endPosition() const noexcept
    {
        return util::Vector3I32(rootNode->getHalfSize());
    }
    util::Vector3I32 maxPosition() const noexcept
    {
        return util::Vector3I32(rootNode->getHalfSize() - 1);
    }
};
}
}
}

#endif /* WORLD_HASHLIFE_WORLD_H_ */
