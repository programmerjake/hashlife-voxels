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
#include "../util/constexpr_array.h"
#include "../util/vector.h"
#include <memory>
#include <list>
#include <iosfwd>
#include <unordered_map>
#include <utility>
#include <type_traits>

namespace programmerjake
{
namespace voxels
{
namespace graphics
{
class RenderBuffer;
struct CommandBuffer;
struct Transform;
}
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
    static constexpr HashlifeNodeBase::LevelType renderCacheNodeLevel = 2;
    struct RenderCacheKey final
    {
        friend class HashlifeWorld;
        static constexpr std::size_t nodeArraySize = 3;
        static constexpr std::int32_t centerSize = HashlifeNodeBase::getSize(renderCacheNodeLevel);
        static constexpr int logBase2OfCenterSize =
            HashlifeNodeBase::getLogBase2OfSize(renderCacheNodeLevel);
        util::Array<HashlifeNodeBase *, nodeArraySize * nodeArraySize * nodeArraySize> nodes;
        HashlifeNodeBase *&getNode(std::size_t x, std::size_t y, std::size_t z) noexcept
        {
            constexprAssert(x < nodeArraySize && y < nodeArraySize && z < nodeArraySize);
            return nodes[nodeArraySize * (nodeArraySize * x + y) + z];
        }
        HashlifeNodeBase *const &getNode(std::size_t x, std::size_t y, std::size_t z) const noexcept
        {
            constexprAssert(x < nodeArraySize && y < nodeArraySize && z < nodeArraySize);
            return nodes[nodeArraySize * (nodeArraySize * x + y) + z];
        }
        block::BlockStepGlobalState blockStepGlobalState;
        explicit RenderCacheKey(const block::BlockStepGlobalState &blockStepGlobalState)
            : nodes{}, blockStepGlobalState(blockStepGlobalState)
        {
        }
        bool operator!=(const RenderCacheKey &rt) const noexcept
        {
            return !operator==(rt);
        }
        bool operator==(const RenderCacheKey &rt) const noexcept
        {
            return nodes == rt.nodes;
        }
        block::Block getBlock(util::Vector3I32 position) const noexcept
        {
            position += util::Vector3I32(centerSize);
            constexprAssert(position.min() >= 0);
            auto index = static_cast<util::Vector3U32>(position) / util::Vector3U32(centerSize);
            position = static_cast<util::Vector3I32>(static_cast<util::Vector3U32>(position)
                                                     % util::Vector3U32(centerSize));
            static_assert(HashlifeNodeBase::levelSize == 2, "");
            static_assert(centerSize >= 2, "");
            position -= util::Vector3I32(centerSize / 2);
            return getNode(index.x, index.y, index.z)->get(position);
        }
        block::BlockSummary getBlockSummary() const noexcept
        {
            block::BlockSummary retval = nodes[0]->blockSummary;
            for(std::size_t i = 1; i < nodes.size(); i++)
            {
                retval += nodes[i]->blockSummary;
            }
            return retval;
        }
    };
    struct RenderCacheKeyHasher
    {
        std::size_t operator()(const RenderCacheKey &key) const
        {
            std::hash<HashlifeNodeBase *> hasher;
            std::size_t retval = 0;
            for(auto *node : key.nodes)
            {
                retval = retval * 1013 + hasher(node);
            }
            return retval;
        }
    };
    struct RenderCacheEntry final
    {
        std::list<const RenderCacheKey *>::iterator renderCacheEntryListIterator;
        std::shared_ptr<graphics::RenderBuffer> renderBuffer;
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
        void dump(std::ostream &os) const
        {
            dumpNode(rootNode, os);
        }
    };
    class RenderCacheEntryReference final
    {
        friend class HashlifeWorld;
        RenderCacheEntryReference(const RenderCacheEntryReference &) = delete;
        RenderCacheEntryReference &operator=(const RenderCacheEntryReference &) = delete;

    private:
        RenderCacheKey key;
        std::shared_ptr<const HashlifeWorld> world;

    public:
        RenderCacheEntryReference(const RenderCacheKey &key,
                                  std::shared_ptr<const HashlifeWorld> world,
                                  PrivateAccessTag)
            : key(key), world(std::move(world))
        {
        }
        ~RenderCacheEntryReference() = default;
        static constexpr std::int32_t centerSize = RenderCacheKey::centerSize;
        static constexpr int logBase2OfCenterSize = RenderCacheKey::logBase2OfCenterSize;
        block::Block get(util::Vector3I32 position) const noexcept
        {
            return key.getBlock(position);
        }
        const block::BlockStepGlobalState &getBlockStepGlobalState() const noexcept
        {
            return key.blockStepGlobalState;
        }
        std::size_t hash() const
        {
            return RenderCacheKeyHasher()(key);
        }
        bool operator==(const RenderCacheEntryReference &rt) const noexcept
        {
            return key == rt.key;
        }
        bool operator!=(const RenderCacheEntryReference &rt) const noexcept
        {
            return key != rt.key;
        }
        block::BlockSummary getBlockSummary() const noexcept
        {
            return key.getBlockSummary();
        }
    };

private:
    HashlifeGarbageCollectedHashtable garbageCollectedHashtable;
    mutable std::list<std::weak_ptr<Snapshot>> snapshots;
    std::list<std::weak_ptr<RenderCacheEntryReference>> renderCacheEntryReferences;
    HashlifeNodeBase *rootNode;
    std::unordered_map<RenderCacheKey, RenderCacheEntry, RenderCacheKeyHasher> renderCache;
    std::list<const RenderCacheKey *> renderCacheEntryList;

public:
    explicit HashlifeWorld(PrivateAccessTag)
        : garbageCollectedHashtable(),
          snapshots(),
          renderCacheEntryReferences(),
          rootNode(garbageCollectedHashtable.getCanonicalEmptyNode(1)),
          renderCache(),
          renderCacheEntryList()
    {
    }
    static constexpr std::size_t defaultRenderCacheTargetEntryCount = 100000;
    void collectGarbage(
        std::size_t garbageCollectTargetNodeCount =
            HashlifeGarbageCollectedHashtable::defaultGarbageCollectTargetNodeCount,
        std::size_t renderCacheTargetEntryCount = defaultRenderCacheTargetEntryCount)
    {
        while(renderCache.size() > renderCacheTargetEntryCount)
        {
            renderCache.erase(*renderCacheEntryList.back());
            renderCacheEntryList.pop_back();
        }
        if(!garbageCollectedHashtable.needGarbageCollect(garbageCollectTargetNodeCount))
            return;
        std::vector<HashlifeNodeBase *> treeRoots;
        treeRoots.reserve(1 + snapshots.size());
        treeRoots.push_back(rootNode);
        for(auto iter = snapshots.begin(); iter != snapshots.end();)
        {
            auto snapshot = iter->lock();
            if(snapshot)
            {
                treeRoots.push_back(snapshot->rootNode);
                ++iter;
            }
            else
            {
                iter = snapshots.erase(iter);
            }
        }
        std::vector<HashlifeNodeBase *const *> rootArrays;
        std::vector<std::size_t> rootArraySizes;
        std::size_t expectedRootArraysSize =
            1 + renderCache.size() + renderCacheEntryReferences.size();
        rootArrays.reserve(expectedRootArraysSize);
        rootArraySizes.reserve(expectedRootArraysSize);
        rootArrays.push_back(treeRoots.data());
        rootArraySizes.push_back(treeRoots.size());
        for(auto *entry : renderCacheEntryList)
        {
            rootArrays.push_back(entry->nodes.data());
            rootArraySizes.push_back(entry->nodes.size());
        }
        for(auto iter = renderCacheEntryReferences.begin();
            iter != renderCacheEntryReferences.end();)
        {
            auto renderCacheEntryReference = iter->lock();
            if(renderCacheEntryReference)
            {
                rootArrays.push_back(renderCacheEntryReference->key.nodes.data());
                rootArraySizes.push_back(renderCacheEntryReference->key.nodes.size());
                ++iter;
            }
            else
            {
                iter = renderCacheEntryReferences.erase(iter);
            }
        }
        constexprAssert(rootArrays.size() == rootArraySizes.size());
        garbageCollectedHashtable.garbageCollect(rootArrays.data(),
                                                 rootArraySizes.data(),
                                                 rootArrays.size(),
                                                 garbageCollectTargetNodeCount);
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
        for(util::Vector3I32 position(0); position.x < HashlifeNodeBase::levelSize; position.x++)
        {
            for(position.y = 0; position.y < HashlifeNodeBase::levelSize; position.y++)
            {
                for(position.z = 0; position.z < HashlifeNodeBase::levelSize; position.z++)
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
                    newChildNode[2 - position.x - 1][2 - position.y - 1][2 - position.z - 1] =
                        static_cast<HashlifeNonleafNode *>(rootNode)->getChildNode(position);
                    newRootNode[position.x][position.y][position.z] =
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
            util::Array<util::Array<util::Array<HashlifeNodeBase *, intermediateSize>,
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
                                    auto childNode = node->getChildNode(index1);
                                    constexprAssert(!childNode->isLeaf());
                                    input[position.x][position.y][position.z] =
                                        static_cast<HashlifeNonleafNode *>(childNode)
                                            ->getChildNode(index2);
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
                                        (position + offsetInEighths + util::Vector3I32(1));
                                    if(outputPosition.x < 0
                                       || outputPosition.x >= HashlifeNodeBase::levelSize
                                                                  * HashlifeNodeBase::levelSize
                                       || outputPosition.y < 0
                                       || outputPosition.y >= HashlifeNodeBase::levelSize
                                                                  * HashlifeNodeBase::levelSize
                                       || outputPosition.z < 0
                                       || outputPosition.z >= HashlifeNodeBase::levelSize
                                                                  * HashlifeNodeBase::levelSize)
                                        continue;
                                    outputPosition /= util::Vector3I32(HashlifeNodeBase::levelSize);
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
                                                    ->getBlock(index2);
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
                                                    ->getChildNode(index2);
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
            HashlifeGarbageCollectedHashtable::defaultGarbageCollectTargetNodeCount,
        std::size_t renderCacheTargetEntryCount = defaultRenderCacheTargetEntryCount)
    {
        collectGarbage(garbageCollectTargetNodeCount, renderCacheTargetEntryCount);
        return step(stepGlobalState);
    }
    block::BlockStepExtraActions step(const block::BlockStepGlobalState &stepGlobalState)
    {
        do
        {
            expandRoot();
        } while(HashlifeNonleafNode::FutureState::getStepSizeInGenerations(rootNode->level)
                < block::BlockStepGlobalState::stepSizeInGenerations);
        auto &futureState = getFilledFutureState(rootNode, stepGlobalState);
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
        constexprAssert(sourcePosition.x + size.x <= blocksArray.size());
        constexprAssert(sourcePosition.y + size.y <= blocksArray[0].size());
        constexprAssert(sourcePosition.z + size.z <= blocksArray[0][0].size());
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
                           || (inputPosition - targetPosition - size).max() >= 0)
                        {
                            blocks[position.x][position.y][position.z] =
                                static_cast<HashlifeLeafNode *>(node)->getBlock(position);
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
                            childNodes[position.x][position.y][position.z] = setBlocks(
                                static_cast<HashlifeNonleafNode *>(node)->getChildNode(position),
                                std::forward<BlocksArray>(blocksArray),
                                minInputPosition + offset,
                                sourcePosition - targetPosition + minInputPosition,
                                endInputPosition - minInputPosition);
                        else
                            childNodes[position.x][position.y][position.z] =
                                static_cast<HashlifeNonleafNode *>(node)->getChildNode(position);
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
        util::Array<util::Array<util::Array<block::Block, 1>, 1>, 1> blocks;
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

private:
    static void dumpNode(HashlifeNodeBase *node, std::ostream &os);

public:
    void dump(std::ostream &os) const
    {
        dumpNode(rootNode, os);
    }

private:
    std::pair<const RenderCacheKey, RenderCacheEntry> &findOrMakeRenderCacheEntry(
        const RenderCacheKey &key)
    {
        auto iter = renderCache.find(key);
        if(iter == renderCache.end())
        {
            iter = std::get<0>(renderCache.emplace(key, RenderCacheEntry()));
            renderCacheEntryList.push_front(&std::get<0>(*iter));
            std::get<1>(*iter).renderCacheEntryListIterator = renderCacheEntryList.begin();
        }
        else
        {
            renderCacheEntryList.splice(renderCacheEntryList.begin(),
                                        renderCacheEntryList,
                                        std::get<1>(*iter).renderCacheEntryListIterator);
        }
        return *iter;
    }

public:
    std::pair<std::shared_ptr<graphics::RenderBuffer>, std::shared_ptr<RenderCacheEntryReference>>
        getRenderCacheEntry(util::Vector3I32 position,
                            const block::BlockStepGlobalState &blockStepGlobalState)
    {
        constexprAssert(position % util::Vector3I32(RenderCacheKey::centerSize)
                        == util::Vector3I32(0));
        while(rootNode->getSize() < RenderCacheKey::centerSize)
        {
            expandRoot();
        }
        RenderCacheKey key(blockStepGlobalState);
        for(std::size_t x = 0; x < RenderCacheKey::nodeArraySize; x++)
        {
            for(std::size_t y = 0; y < RenderCacheKey::nodeArraySize; y++)
            {
                for(std::size_t z = 0; z < RenderCacheKey::nodeArraySize; z++)
                {
                    auto nodePosition =
                        position - util::Vector3I32(RenderCacheKey::centerSize)
                        + util::Vector3I32(RenderCacheKey::centerSize) * util::Vector3I32(x, y, z);
                    key.getNode(x, y, z) =
                        rootNode->isPositionInside(nodePosition) ?
                            rootNode->get(nodePosition, renderCacheNodeLevel) :
                            garbageCollectedHashtable.getCanonicalEmptyNode(renderCacheNodeLevel);
                }
            }
        }
        auto &entry = findOrMakeRenderCacheEntry(key);
        if(std::get<1>(entry).renderBuffer)
            return {std::get<1>(entry).renderBuffer, nullptr};
        auto renderCacheEntryReference = std::make_shared<RenderCacheEntryReference>(
            key, shared_from_this(), PrivateAccessTag());
        return {nullptr, std::move(renderCacheEntryReference)};
    }
    void setRenderCacheEntry(
        const std::shared_ptr<RenderCacheEntryReference> &renderCacheEntryReference,
        std::shared_ptr<graphics::RenderBuffer> renderBuffer)
    {
        constexprAssert(renderCacheEntryReference);
        auto &entryValue = std::get<1>(findOrMakeRenderCacheEntry(renderCacheEntryReference->key));
        entryValue.renderBuffer = std::move(renderBuffer);
    }
    static std::shared_ptr<graphics::RenderBuffer> renderRenderCacheEntry(
        const std::shared_ptr<RenderCacheEntryReference> &renderCacheEntryReference);
    void renderView(std::shared_ptr<graphics::RenderBuffer>(*handleUnrenderedChunk)(
                        void *arg,
                        std::shared_ptr<RenderCacheEntryReference> renderCacheEntryReference),
                    void *handleUnrenderedChunkArg,
                    util::Vector3F viewLocation,
                    float viewDistance,
                    const std::shared_ptr<graphics::CommandBuffer> &commandBuffer,
                    const graphics::Transform &viewTransform,
                    const graphics::Transform &projectionTransform,
                    const block::BlockStepGlobalState &blockStepGlobalState);
    template <typename HandleUnrenderedChunk>
    void renderView(HandleUnrenderedChunk &&handleUnrenderedChunk,
                    util::Vector3F viewLocation,
                    float viewDistance,
                    const std::shared_ptr<graphics::CommandBuffer> &commandBuffer,
                    const graphics::Transform &viewTransform,
                    const graphics::Transform &projectionTransform,
                    const block::BlockStepGlobalState &blockStepGlobalState)
    {
        renderView(
            [](void *arg, std::shared_ptr<RenderCacheEntryReference> renderCacheEntryReference)
                -> std::shared_ptr<graphics::RenderBuffer>
            {
                return std::forward<HandleUnrenderedChunk>(
                    *static_cast<typename std::decay<HandleUnrenderedChunk>::type *>(arg))(
                    std::move(renderCacheEntryReference));
            },
            const_cast<char *>(&reinterpret_cast<const volatile char &>(handleUnrenderedChunk)),
            viewLocation,
            viewDistance,
            commandBuffer,
            viewTransform,
            projectionTransform,
            blockStepGlobalState);
    }
};
}
}
}

#endif /* WORLD_HASHLIFE_WORLD_H_ */
