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
#include "../util/function_reference.h"
#include <memory>
#include <list>
#include <iosfwd>
#include <unordered_map>
#include <utility>
#include <type_traits>
#include <mutex>

namespace programmerjake
{
namespace voxels
{
namespace graphics
{
class RenderBuffer;
class ReadableRenderBuffer;
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
    static constexpr HashlifeNodeBase::LevelType renderCacheNodeLevel = 3;
    struct RenderCacheKey final
    {
        friend class HashlifeWorld;
        static constexpr std::int32_t nodeArraySize = 3;
        static_assert(nodeArraySize == 3, "");
        static constexpr std::int32_t centerSize = HashlifeNodeBase::getSize(renderCacheNodeLevel);
        static constexpr int logBase2OfCenterSize =
            HashlifeNodeBase::getLogBase2OfSize(renderCacheNodeLevel);
        util::Array<std::shared_ptr<const HashlifeNodeBase>,
                    nodeArraySize * nodeArraySize * nodeArraySize> nodes;
        std::shared_ptr<const HashlifeNodeBase> &getNode(std::size_t x,
                                                         std::size_t y,
                                                         std::size_t z) noexcept
        {
            constexprAssert(x < nodeArraySize && y < nodeArraySize && z < nodeArraySize);
            return nodes[nodeArraySize * (nodeArraySize * x + y) + z];
        }
        const std::shared_ptr<const HashlifeNodeBase> &getNode(std::size_t x,
                                                               std::size_t y,
                                                               std::size_t z) const noexcept
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
        template <typename BlocksArray>
        void getBlocks(BlocksArray &&blocksArray,
                       util::Vector3I32 keyRelativePosition,
                       util::Vector3I32 arrayPosition,
                       util::Vector3I32 size) const
        {
            keyRelativePosition += util::Vector3I32(centerSize);
            constexprAssert(size.min() >= 0);
            if(size.min() <= 0)
                return;
            util::Vector3I32 minPosition(0);
            util::Vector3I32 maxPosition(centerSize * nodeArraySize - 1);
            for(auto position = keyRelativePosition; position.x < keyRelativePosition.x + size.x;
                position.x++)
            {
                if(position.x >= minPosition.x && position.x <= maxPosition.x)
                {
                    position.x = maxPosition.x + 1;
                    continue;
                }
                for(position.y = keyRelativePosition.y; position.y < keyRelativePosition.y + size.y;
                    position.y++)
                {
                    if(position.y >= minPosition.y && position.y <= maxPosition.y)
                    {
                        position.y = maxPosition.y + 1;
                        continue;
                    }
                    for(position.z = keyRelativePosition.z;
                        position.z < keyRelativePosition.z + size.z;
                        position.z++)
                    {
                        if(position.z >= minPosition.z && position.z <= maxPosition.z)
                        {
                            position.z = maxPosition.z + 1;
                            continue;
                        }
                        const auto blocksArrayPosition =
                            position + arrayPosition - keyRelativePosition;
                        std::forward<BlocksArray>(
                            blocksArray)[blocksArrayPosition.x][blocksArrayPosition
                                                                    .y][blocksArrayPosition.z] =
                            block::Block();
                    }
                }
            }
            for(util::Vector3I32 index(0); index.x < nodeArraySize; index.x++)
            {
                for(index.y = 0; index.y < nodeArraySize; index.y++)
                {
                    for(index.z = 0; index.z < nodeArraySize; index.z++)
                    {
                        auto position = index * util::Vector3I32(centerSize);
                        static_assert(HashlifeNodeBase::levelSize == 2, "");
                        static_assert(centerSize >= 2, "");
                        auto startArrayPosition =
                            max(position - keyRelativePosition + arrayPosition, arrayPosition);
                        auto endArrayPosition =
                            max(startArrayPosition,
                                min(position - keyRelativePosition + arrayPosition
                                        + util::Vector3I32(centerSize),
                                    arrayPosition + size));
                        getBlocksImplementation(getNode(index.x, index.y, index.z),
                                                std::forward<BlocksArray>(blocksArray),
                                                startArrayPosition - arrayPosition
                                                    + keyRelativePosition
                                                    - util::Vector3I32(centerSize / 2) - position,
                                                startArrayPosition,
                                                endArrayPosition - startArrayPosition);
                    }
                }
            }
        }
    };
    struct RenderCacheKeyHasher
    {
        std::size_t operator()(const RenderCacheKey &key) const
        {
            std::hash<std::shared_ptr<const HashlifeNodeBase>> hasher;
            std::size_t retval = 0;
            for(auto &node : key.nodes)
            {
                retval = retval * 1013 + hasher(node);
            }
            return retval;
        }
    };
    struct RenderCacheEntry final
    {
        std::list<const RenderCacheKey *>::iterator renderCacheEntryListIterator;
        std::shared_ptr<graphics::ReadableRenderBuffer> renderBuffer;
    };

private:
    static block::Block getBlock(std::shared_ptr<const HashlifeNodeBase> rootNode,
                                 util::Vector3I32 position) noexcept
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
        std::shared_ptr<const HashlifeNodeBase> rootNode;
        std::shared_ptr<const HashlifeWorld> world;

    public:
        Snapshot(std::shared_ptr<const HashlifeNodeBase> rootNode, PrivateAccessTag)
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
        template <typename BlocksArray>
        void getBlocks(BlocksArray &&blocksArray,
                       util::Vector3I32 worldPosition,
                       util::Vector3I32 arrayPosition,
                       util::Vector3I32 size) const
        {
            getBlocks(rootNode,
                      std::forward<BlocksArray>(blocksArray),
                      worldPosition,
                      arrayPosition,
                      size);
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
        template <typename BlocksArray>
        void getBlocks(BlocksArray &&blocksArray,
                       util::Vector3I32 entryRelativePosition,
                       util::Vector3I32 arrayPosition,
                       util::Vector3I32 size) const
        {
            key.getBlocks(
                std::forward<BlocksArray>(blocksArray), entryRelativePosition, arrayPosition, size);
        }
    };

private:
    HashlifeGarbageCollectedHashtable garbageCollectedHashtable;
    std::list<std::weak_ptr<RenderCacheEntryReference>> renderCacheEntryReferences;
    std::shared_ptr<const HashlifeNodeBase> rootNode;
    std::unordered_map<RenderCacheKey, RenderCacheEntry, RenderCacheKeyHasher> renderCache;
    std::list<const RenderCacheKey *> renderCacheEntryList;

public:
    explicit HashlifeWorld(PrivateAccessTag);
    static constexpr std::size_t defaultRenderCacheTargetEntryCount = 100000;
    void collectGarbage(
        std::size_t garbageCollectTargetNodeCount =
            HashlifeGarbageCollectedHashtable::defaultGarbageCollectTargetNodeCount,
        std::size_t renderCacheTargetEntryCount = defaultRenderCacheTargetEntryCount);
    std::shared_ptr<const Snapshot> makeSnapshot() const
    {
        auto retval = std::make_shared<Snapshot>(rootNode, PrivateAccessTag());
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
    void expandRoot();
    const HashlifeNonleafNode::FutureState &getFilledFutureState(
        std::shared_ptr<const HashlifeNodeBase> nodeIn,
        const block::BlockStepGlobalState &stepGlobalState);

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
    std::shared_ptr<const HashlifeNodeBase> setBlocks(std::shared_ptr<const HashlifeNodeBase> node,
                                                      BlocksArray &&blocksArray,
                                                      util::Vector3I32 worldPosition,
                                                      util::Vector3I32 arrayPosition,
                                                      util::Vector3I32 size)
    {
        constexprAssert(size.min() >= 0);
        constexprAssert(arrayPosition.x + size.x <= static_cast<std::int32_t>(blocksArray.size()));
        constexprAssert(arrayPosition.y + size.y
                        <= static_cast<std::int32_t>(blocksArray[0].size()));
        constexprAssert(arrayPosition.z + size.z
                        <= static_cast<std::int32_t>(blocksArray[0][0].size()));
        if(size.min() == 0)
            return node;
        constexprAssert(node->isPositionInside(worldPosition));
        constexprAssert(node->isPositionInside(worldPosition + size - util::Vector3I32(1)));
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
                        if((inputPosition - worldPosition).min() < 0
                           || (inputPosition - worldPosition - size).max() >= 0)
                        {
                            blocks[position.x][position.y][position.z] =
                                getAsLeaf(node.get())->getBlock(position);
                        }
                        else
                        {
                            const auto blocksArrayPosition =
                                inputPosition + arrayPosition - worldPosition;
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
                        minInputPosition = max(minInputPosition, worldPosition);
                        endInputPosition = min(endInputPosition, worldPosition + size);
                        if((endInputPosition - minInputPosition).min() > 0)
                            childNodes[position.x][position.y][position.z] =
                                setBlocks(getAsNonleaf(node.get())->getChildNode(position),
                                          std::forward<BlocksArray>(blocksArray),
                                          minInputPosition + offset,
                                          arrayPosition - worldPosition + minInputPosition,
                                          endInputPosition - minInputPosition);
                        else
                            childNodes[position.x][position.y][position.z] =
                                getAsNonleaf(node.get())->getChildNode(position);
                    }
                }
            }
            return garbageCollectedHashtable.findOrAddNode(HashlifeNonleafNode(childNodes));
        }
    }

public:
    template <typename BlocksArray>
    void setBlocks(BlocksArray &&blocksArray,
                   util::Vector3I32 worldPosition,
                   util::Vector3I32 arrayPosition,
                   util::Vector3I32 size)
    {
        constexprAssert(size.min() >= 0);
        if(size.min() <= 0)
            return;
        while(!rootNode->isPositionInside(worldPosition)
              || !rootNode->isPositionInside(worldPosition + size - util::Vector3I32(1)))
            expandRoot();
        rootNode = setBlocks(
            rootNode, std::forward<BlocksArray>(blocksArray), worldPosition, arrayPosition, size);
    }

private:
    template <typename BlocksArray>
    static void getBlocksImplementation(const std::shared_ptr<const HashlifeNodeBase> &node,
                                        BlocksArray &&blocksArray,
                                        util::Vector3I32 worldPosition,
                                        util::Vector3I32 arrayPosition,
                                        util::Vector3I32 size)
    {
        constexprAssert(size.min() >= 0);
        constexprAssert(arrayPosition.x + size.x <= static_cast<std::int32_t>(blocksArray.size()));
        constexprAssert(arrayPosition.y + size.y
                        <= static_cast<std::int32_t>(blocksArray[0].size()));
        constexprAssert(arrayPosition.z + size.z
                        <= static_cast<std::int32_t>(blocksArray[0][0].size()));
        if(size.min() == 0)
            return;
        constexprAssert(node->isPositionInside(worldPosition));
        constexprAssert(node->isPositionInside(worldPosition + size - util::Vector3I32(1)));
        if(node->isLeaf())
        {
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
                        if((inputPosition - worldPosition).min() >= 0
                           && (inputPosition - worldPosition - size).max() < 0)
                        {
                            const auto blocksArrayPosition =
                                inputPosition + arrayPosition - worldPosition;
                            std::forward<BlocksArray>(
                                blocksArray)[blocksArrayPosition.x][blocksArrayPosition
                                                                        .y][blocksArrayPosition.z] =
                                getAsLeaf(node.get())->getBlock(position);
                        }
                    }
                }
            }
        }
        else
        {
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
                        minInputPosition = max(minInputPosition, worldPosition);
                        endInputPosition = min(endInputPosition, worldPosition + size);
                        if((endInputPosition - minInputPosition).min() > 0)
                            getBlocksImplementation(
                                getAsNonleaf(node.get())->getChildNode(position),
                                std::forward<BlocksArray>(blocksArray),
                                minInputPosition + offset,
                                arrayPosition - worldPosition + minInputPosition,
                                endInputPosition - minInputPosition);
                    }
                }
            }
        }
    }
    template <typename BlocksArray>
    static void getBlocks(const std::shared_ptr<const HashlifeNodeBase> &node,
                          BlocksArray &&blocksArray,
                          util::Vector3I32 worldPosition,
                          util::Vector3I32 arrayPosition,
                          util::Vector3I32 size)
    {
        constexprAssert(size.min() >= 0);
        if(size.min() <= 0)
            return;
        util::Vector3I32 minPosition(-node->getHalfSize());
        util::Vector3I32 maxPosition(node->getHalfSize() - 1);
        for(auto position = worldPosition; position.x < worldPosition.x + size.x; position.x++)
        {
            if(position.x >= minPosition.x && position.x <= maxPosition.x)
            {
                position.x = maxPosition.x + 1;
                continue;
            }
            for(position.y = worldPosition.y; position.y < worldPosition.y + size.y; position.y++)
            {
                if(position.y >= minPosition.y && position.y <= maxPosition.y)
                {
                    position.y = maxPosition.y + 1;
                    continue;
                }
                for(position.z = worldPosition.z; position.z < worldPosition.z + size.z;
                    position.z++)
                {
                    if(position.z >= minPosition.z && position.z <= maxPosition.z)
                    {
                        position.z = maxPosition.z + 1;
                        continue;
                    }
                    const auto blocksArrayPosition = position + arrayPosition - worldPosition;
                    std::forward<BlocksArray>(
                        blocksArray)[blocksArrayPosition.x][blocksArrayPosition
                                                                .y][blocksArrayPosition.z] =
                        block::Block();
                }
            }
        }
        getBlocksImplementation(
            node, std::forward<BlocksArray>(blocksArray), worldPosition, arrayPosition, size);
    }

public:
    template <typename BlocksArray>
    void getBlocks(BlocksArray &&blocksArray,
                   util::Vector3I32 worldPosition,
                   util::Vector3I32 arrayPosition,
                   util::Vector3I32 size) const
    {
        getBlocks(
            rootNode, std::forward<BlocksArray>(blocksArray), worldPosition, arrayPosition, size);
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
    static void dumpNode(std::shared_ptr<const HashlifeNodeBase> node, std::ostream &os);

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
    std::pair<std::shared_ptr<graphics::ReadableRenderBuffer>,
              std::shared_ptr<RenderCacheEntryReference>>
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
                            rootNode->get(nodePosition, renderCacheNodeLevel)->shared_from_this() :
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
        std::shared_ptr<graphics::ReadableRenderBuffer> renderBuffer)
    {
        constexprAssert(renderCacheEntryReference);
        auto &entryValue = std::get<1>(findOrMakeRenderCacheEntry(renderCacheEntryReference->key));
        entryValue.renderBuffer = std::move(renderBuffer);
    }
    static std::shared_ptr<graphics::ReadableRenderBuffer> renderRenderCacheEntry(
        const std::shared_ptr<RenderCacheEntryReference> &renderCacheEntryReference);
    class GPURenderBufferCache final
    {
    public:
        static constexpr int logBase2OfSizeInChunks =
            4 - RenderCacheEntryReference::logBase2OfCenterSize;
        static_assert(logBase2OfSizeInChunks >= 0, "");
        static constexpr int logBase2OfSizeInBlocks =
            logBase2OfSizeInChunks + RenderCacheEntryReference::logBase2OfCenterSize;
        static constexpr std::int32_t sizeInChunks = 1LL << logBase2OfSizeInChunks;
        static constexpr std::int32_t sizeInBlocks = 1LL << logBase2OfSizeInBlocks;
        static_assert(sizeInBlocks == sizeInChunks * RenderCacheEntryReference::centerSize, "");
        struct SourceRenderBuffers final
        {
            util::Array<util::Array<util::Array<std::shared_ptr<graphics::ReadableRenderBuffer>,
                                                sizeInChunks>,
                                    sizeInChunks>,
                        sizeInChunks> renderBuffers;
            std::shared_ptr<graphics::RenderBuffer> render() const;
        };
        struct Entry final
        {
            const util::Vector3I32 position;
            SourceRenderBuffers sourceRenderBuffers;
            std::shared_ptr<graphics::RenderBuffer> gpuRenderBuffer;
            explicit Entry(util::Vector3I32 position)
                : position(position), sourceRenderBuffers(), gpuRenderBuffer(nullptr)
            {
            }
        };

    private:
        static constexpr std::size_t sliceCount = 8191;
        struct Slice final
        {
            std::unordered_map<util::Vector3I32, std::shared_ptr<Entry>> entries;
            mutable std::mutex lock;
        };
        util::Array<Slice, sliceCount> slices;
        static std::size_t getSliceIndex(util::Vector3I32 location)
        {
            return std::hash<util::Vector3I32>()(location) % sliceCount;
        }

    public:
        std::shared_ptr<Entry> getEntry(util::Vector3I32 location) const
        {
            auto &slice = slices[getSliceIndex(location)];
            std::unique_lock<std::mutex> lockIt(slice.lock);
            auto iter = slice.entries.find(location);
            if(iter != slice.entries.end())
                return std::get<1>(*iter);
            return nullptr;
        }
        void setEntry(util::Vector3I32 location, std::shared_ptr<Entry> entry)
        {
            auto &slice = slices[getSliceIndex(location)];
            std::unique_lock<std::mutex> lockIt(slice.lock);
            slice.entries[location] = std::move(entry);
        }
        void renderView(util::Vector3F viewLocation,
                        float viewDistance,
                        const std::shared_ptr<graphics::CommandBuffer> &commandBuffer,
                        const graphics::Transform &viewTransform,
                        const graphics::Transform &projectionTransform) const;
    };
    void updateView(
        util::FunctionReference<std::shared_ptr<graphics::ReadableRenderBuffer>(
            std::shared_ptr<RenderCacheEntryReference> renderCacheEntryReference)>
            handleUnrenderedChunk,
        util::FunctionReference<void(std::shared_ptr<GPURenderBufferCache::Entry> entry)>
            handleUpdateGPURenderBuffer,
        util::Vector3F viewLocation,
        float viewDistance,
        const block::BlockStepGlobalState &blockStepGlobalState,
        GPURenderBufferCache &gpuRenderBufferCache);
};
}
}
}

#endif /* WORLD_HASHLIFE_WORLD_H_ */
