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
#include "hashlife_world.h"
#include "../graphics/render.h"
#include "../lighting/lighting.h"
#include "../util/constexpr_array.h"
#include "../util/constexpr_assert.h"
#include "../util/enum.h"
#include "../graphics/driver.h"
#include <ostream>
#include <unordered_map>
#include <deque>
#include <memory>

namespace programmerjake
{
namespace voxels
{
namespace world
{
void HashlifeWorld::dumpNode(HashlifeNodeBase *node, std::ostream &os)
{
    std::unordered_map<HashlifeNodeBase *, std::size_t> nodeNumbers;
    std::deque<HashlifeNodeBase *> worklist;
    worklist.push_back(node);
    nodeNumbers[node] = 0;
    while(!worklist.empty())
    {
        auto currentNode = worklist.front();
        worklist.pop_front();
        os << "#" << nodeNumbers.at(currentNode) << ": (" << static_cast<const void *>(currentNode)
           << ")\n    level = " << static_cast<unsigned>(currentNode->level) << "\n";
        if(currentNode->isLeaf())
        {
            for(util::Vector3I32 position(0); position.x < HashlifeNodeBase::levelSize;
                position.x++)
            {
                for(position.y = 0; position.y < HashlifeNodeBase::levelSize; position.y++)
                {
                    for(position.z = 0; position.z < HashlifeNodeBase::levelSize; position.z++)
                    {
                        auto block =
                            static_cast<HashlifeLeafNode *>(currentNode)->getBlock(position);
                        auto blockDescriptor = block::BlockDescriptor::get(block.getBlockKind());
                        os << "    [" << position.x << "][" << position.y << "][" << position.z
                           << "] = <" << static_cast<unsigned>(block.getDirectSkylight()) << ", "
                           << static_cast<unsigned>(block.getIndirectSkylight()) << ", "
                           << static_cast<unsigned>(block.getIndirectArtificalLight()) << "> ";
                        if(blockDescriptor)
                            os << blockDescriptor->name;
                        else
                            os << "<empty>";
                        os << "\n";
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
                        auto childNode =
                            static_cast<HashlifeNonleafNode *>(currentNode)->getChildNode(position);
                        auto iter = nodeNumbers.find(childNode);
                        if(iter == nodeNumbers.end())
                        {
                            iter = std::get<0>(nodeNumbers.emplace(childNode, nodeNumbers.size()));
                            worklist.push_back(childNode);
                        }
                        os << "    [" << position.x << "][" << position.y << "][" << position.z
                           << "] = #" << std::get<1>(*iter) << "\n";
                    }
                }
            }
        }
        os << "\n";
    }
    os.flush();
}

std::shared_ptr<graphics::ReadableRenderBuffer> HashlifeWorld::renderRenderCacheEntry(
    const std::shared_ptr<RenderCacheEntryReference> &renderCacheEntryReference)
{
    constexprAssert(renderCacheEntryReference);
    if(!renderCacheEntryReference->getBlockSummary().rendersAnything())
        return graphics::EmptyRenderBuffer::get();
    constexpr std::int32_t blockLightingArraySize =
        RenderCacheEntryReference::centerSize + 2; // one extra block on each side
    struct State final
    {
        util::Array<util::Array<util::Array<lighting::BlockLighting, blockLightingArraySize>,
                                blockLightingArraySize>,
                    blockLightingArraySize> blockLightingArray;
    };
    std::unique_ptr<State> state(new State());
    constexpr std::int32_t blocksSize = 3;
    for(std::int32_t x = 0; x < blockLightingArraySize; x++)
    {
        for(std::int32_t y = 0; y < blockLightingArraySize; y++)
        {
            for(std::int32_t z = 0; z < blockLightingArraySize; z++)
            {
                auto blockLightingPosition = util::Vector3I32(x, y, z) - util::Vector3I32(1);
                util::Array<util::Array<util::Array<std::pair<lighting::LightProperties,
                                                              lighting::Lighting>,
                                                    blocksSize>,
                                        blocksSize>,
                            blocksSize> blocks;
                for(std::int32_t x2 = 0; x2 < blocksSize; x2++)
                {
                    for(std::int32_t y2 = 0; y2 < blocksSize; y2++)
                    {
                        for(std::int32_t z2 = 0; z2 < blocksSize; z2++)
                        {
                            auto blockPosition = blockLightingPosition
                                                 + util::Vector3I32(x2, y2, z2)
                                                 - util::Vector3I32(1);
                            auto block = renderCacheEntryReference->get(blockPosition);
                            blocks[x2][y2][z2] = {
                                block::BlockDescriptor::getLightProperties(block.getBlockKind()),
                                block.getLighting()};
                        }
                    }
                }
                auto &blockLighting = state->blockLightingArray[x][y][z];
                blockLighting = lighting::BlockLighting(
                    blocks,
                    renderCacheEntryReference->getBlockStepGlobalState().lightingGlobalProperties);
            }
        }
    }
    auto renderBuffer = std::make_shared<graphics::MemoryRenderBuffer>();
    for(std::int32_t x = 0; x < RenderCacheEntryReference::centerSize; x++)
    {
        for(std::int32_t y = 0; y < RenderCacheEntryReference::centerSize; y++)
        {
            for(std::int32_t z = 0; z < RenderCacheEntryReference::centerSize; z++)
            {
                auto block = renderCacheEntryReference->get(util::Vector3I32(x, y, z));
                if(!block.getBlockKind())
                    continue;
                auto *blockDescriptor = block::BlockDescriptor::get(block.getBlockKind());
                constexprAssert(blockDescriptor);
                block::BlockStepInput blockStepInput;
                constexprAssert(blockStepInput.blocks.size() == blocksSize);
                constexprAssert(blockStepInput.blocks[0].size() == blocksSize);
                constexprAssert(blockStepInput.blocks[0][0].size() == blocksSize);
                for(std::int32_t x2 = 0; x2 < blocksSize; x2++)
                {
                    for(std::int32_t y2 = 0; y2 < blocksSize; y2++)
                    {
                        for(std::int32_t z2 = 0; z2 < blocksSize; z2++)
                        {
                            blockStepInput.blocks[x2][y2][z2] = renderCacheEntryReference->get(
                                util::Vector3I32(x, y, z) + util::Vector3I32(x2, y2, z2)
                                - util::Vector3I32(1));
                        }
                    }
                }
                util::EnumArray<const lighting::BlockLighting *, block::BlockFace>
                    blockLightingForFaces;
                for(block::BlockFace blockFace : util::EnumTraits<block::BlockFace>::values)
                {
                    auto blockLightingPosition =
                        util::Vector3I32(x, y, z) + getDirection(blockFace);
                    blockLightingForFaces[blockFace] =
                        &state->blockLightingArray[blockLightingPosition.x
                                                   + 1][blockLightingPosition.y
                                                        + 1][blockLightingPosition.z + 1];
                }
                blockDescriptor->render(*renderBuffer,
                                        blockStepInput,
                                        renderCacheEntryReference->getBlockStepGlobalState(),
                                        blockLightingForFaces,
                                        state->blockLightingArray[x + 1][y + 1][z + 1],
                                        graphics::Transform::translate(x, y, z));
            }
        }
    }
    state.reset();
    renderBuffer->finish();
    return renderBuffer;
}

std::shared_ptr<graphics::RenderBuffer>
    HashlifeWorld::GPURenderBufferCache::SourceRenderBuffers::render() const
{
    std::shared_ptr<graphics::RenderBuffer> retval;
    {
        graphics::MemoryRenderBuffer memoryRenderBuffer;
        for(util::Vector3I32 index(0); index.x < GPURenderBufferCache::sizeInChunks; index.x++)
        {
            for(index.y = 0; index.y < GPURenderBufferCache::sizeInChunks; index.y++)
            {
                for(index.z = 0; index.z < GPURenderBufferCache::sizeInChunks; index.z++)
                {
                    auto &sourceRenderBuffer = renderBuffers[index.x][index.y][index.z];
                    if(sourceRenderBuffer)
                    {
                        memoryRenderBuffer.appendBuffer(
                            *sourceRenderBuffer,
                            graphics::Transform::translate(static_cast<util::Vector3F>(
                                index * util::Vector3I32(RenderCacheEntryReference::centerSize))));
                    }
                }
            }
        }
        retval = graphics::RenderBuffer::makeGPUBuffer(memoryRenderBuffer.getTriangleCounts());
        retval->appendBuffer(memoryRenderBuffer);
    }
    retval->finish();
    return retval;
}

void HashlifeWorld::GPURenderBufferCache::renderView(
    util::Vector3F viewLocation,
    float viewDistance,
    const std::shared_ptr<graphics::CommandBuffer> &commandBuffer,
    const graphics::Transform &viewTransform,
    const graphics::Transform &projectionTransform) const
{
    constexprAssert(viewDistance >= 0);
    auto minViewLocation =
        static_cast<util::Vector3I32>(viewLocation - util::Vector3F(viewDistance));
    auto maxViewLocation =
        static_cast<util::Vector3I32>(viewLocation + util::Vector3F(viewDistance));
    util::Vector3I32 minRenderBufferPosition =
        minViewLocation
        & ~(util::Vector3I32((1LL << GPURenderBufferCache::logBase2OfSizeInBlocks) - 1));
    util::Vector3I32 maxRenderBufferPosition =
        maxViewLocation
        & ~(util::Vector3I32((1LL << GPURenderBufferCache::logBase2OfSizeInBlocks) - 1));
    for(util::Vector3I32 renderBufferPosition = minRenderBufferPosition;
        renderBufferPosition.x < maxRenderBufferPosition.x;
        renderBufferPosition.x += GPURenderBufferCache::sizeInBlocks)
    {
        for(renderBufferPosition.y = minRenderBufferPosition.y;
            renderBufferPosition.y < maxRenderBufferPosition.y;
            renderBufferPosition.y += GPURenderBufferCache::sizeInBlocks)
        {
            for(renderBufferPosition.z = minRenderBufferPosition.z;
                renderBufferPosition.z < maxRenderBufferPosition.z;
                renderBufferPosition.z += GPURenderBufferCache::sizeInBlocks)
            {
                auto entry = getEntry(renderBufferPosition);
                if(entry && entry->gpuRenderBuffer)
                {
                    commandBuffer->appendRenderCommand(
                        entry->gpuRenderBuffer,
                        graphics::Transform::translate(
                            static_cast<util::Vector3F>(renderBufferPosition) - viewLocation),
                        viewTransform,
                        projectionTransform);
                }
            }
        }
    }
}

void HashlifeWorld::updateView(
    std::shared_ptr<graphics::ReadableRenderBuffer>(*handleUnrenderedChunk)(
        void *arg, std::shared_ptr<RenderCacheEntryReference> renderCacheEntryReference),
    void *handleUnrenderedChunkArg,
    void (*handleUpdateGPURenderBuffer)(void *arg,
                                        std::shared_ptr<GPURenderBufferCache::Entry> entry),
    void *handleUpdateGPURenderBufferArg,
    util::Vector3F viewLocation,
    float viewDistance,
    const block::BlockStepGlobalState &blockStepGlobalState,
    GPURenderBufferCache &gpuRenderBufferCache)
{
    constexprAssert(viewDistance >= 0);
    auto minViewLocation =
        static_cast<util::Vector3I32>(viewLocation - util::Vector3F(viewDistance));
    auto maxViewLocation =
        static_cast<util::Vector3I32>(viewLocation + util::Vector3F(viewDistance));
    util::Vector3I32 minRenderBufferPosition =
        minViewLocation
        & ~(util::Vector3I32((1LL << GPURenderBufferCache::logBase2OfSizeInBlocks) - 1));
    util::Vector3I32 maxRenderBufferPosition =
        maxViewLocation
        & ~(util::Vector3I32((1LL << GPURenderBufferCache::logBase2OfSizeInBlocks) - 1));
    for(util::Vector3I32 renderBufferPosition = minRenderBufferPosition;
        renderBufferPosition.x < maxRenderBufferPosition.x;
        renderBufferPosition.x += GPURenderBufferCache::sizeInBlocks)
    {
        for(renderBufferPosition.y = minRenderBufferPosition.y;
            renderBufferPosition.y < maxRenderBufferPosition.y;
            renderBufferPosition.y += GPURenderBufferCache::sizeInBlocks)
        {
            for(renderBufferPosition.z = minRenderBufferPosition.z;
                renderBufferPosition.z < maxRenderBufferPosition.z;
                renderBufferPosition.z += GPURenderBufferCache::sizeInBlocks)
            {
                auto entry = gpuRenderBufferCache.getEntry(renderBufferPosition);
                bool anyChanges = false;
                if(!entry)
                {
                    entry = std::make_shared<GPURenderBufferCache::Entry>(renderBufferPosition);
                    anyChanges = true;
                }
                for(util::Vector3I32 index(0); index.x < GPURenderBufferCache::sizeInChunks;
                    index.x++)
                {
                    for(index.y = 0; index.y < GPURenderBufferCache::sizeInChunks; index.y++)
                    {
                        for(index.z = 0; index.z < GPURenderBufferCache::sizeInChunks; index.z++)
                        {
                            util::Vector3I32 chunk =
                                renderBufferPosition
                                + index * util::Vector3I32(RenderCacheEntryReference::centerSize);
                            auto renderCacheEntry =
                                getRenderCacheEntry(chunk, blockStepGlobalState);
                            auto &renderBuffer = std::get<0>(renderCacheEntry);
                            if(!renderBuffer)
                            {
                                renderBuffer = handleUnrenderedChunk(handleUnrenderedChunkArg,
                                                                     std::get<1>(renderCacheEntry));
                                if(renderBuffer)
                                    setRenderCacheEntry(std::get<1>(renderCacheEntry),
                                                        renderBuffer);
                                else
                                    continue;
                            }
                            if(renderBuffer
                               != entry->sourceRenderBuffers
                                      .renderBuffers[index.x][index.y][index.z])
                            {
                                if(!anyChanges)
                                {
                                    // duplicate so we're not modifying concurrently with read
                                    entry = std::make_shared<GPURenderBufferCache::Entry>(*entry);
                                    anyChanges = true;
                                }
                                entry->sourceRenderBuffers
                                    .renderBuffers[index.x][index.y][index.z] =
                                    std::move(renderBuffer);
                            }
                        }
                    }
                }
                if(anyChanges || entry->gpuRenderBuffer == nullptr)
                    handleUpdateGPURenderBuffer(handleUpdateGPURenderBufferArg, entry);
                if(anyChanges)
                    gpuRenderBufferCache.setEntry(renderBufferPosition, std::move(entry));
            }
        }
    }
}
}
}
}
