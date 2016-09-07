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

std::shared_ptr<graphics::RenderBuffer> HashlifeWorld::renderRenderCacheEntry(
    const std::shared_ptr<RenderCacheEntryReference> &renderCacheEntryReference)
{
    constexprAssert(renderCacheEntryReference);
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
    std::shared_ptr<graphics::RenderBuffer> retval;
    {
        graphics::MemoryRenderBuffer memoryRenderBuffer;
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
                    blockDescriptor->render(memoryRenderBuffer,
                                            blockStepInput,
                                            renderCacheEntryReference->getBlockStepGlobalState(),
                                            blockLightingForFaces,
                                            state->blockLightingArray[x + 1][y + 1][z + 1],
                                            graphics::Transform::translate(x, y, z));
                }
            }
        }
        state.reset();
        retval = graphics::RenderBuffer::makeGPUBuffer(memoryRenderBuffer.getTriangleCounts());
        retval->appendBuffer(memoryRenderBuffer);
    }
    retval->finish();
    return retval;
}

void HashlifeWorld::renderView(
    std::shared_ptr<graphics::RenderBuffer>(*handleUnrenderedChunk)(
        void *arg, std::shared_ptr<RenderCacheEntryReference> renderCacheEntryReference),
    void *handleUnrenderedChunkArg,
    util::Vector3F viewLocation,
    float viewDistance,
    const std::shared_ptr<graphics::CommandBuffer> &commandBuffer,
    const graphics::Transform &viewTransform,
    const graphics::Transform &projectionTransform,
    const block::BlockStepGlobalState &blockStepGlobalState)
{
    constexprAssert(viewDistance >= 0);
    auto minViewLocation =
        static_cast<util::Vector3I32>(viewLocation - util::Vector3F(viewDistance));
    auto maxViewLocation =
        static_cast<util::Vector3I32>(viewLocation + util::Vector3F(viewDistance));
    util::Vector3I32 minChunk =
        minViewLocation
        & ~(util::Vector3I32((1LL << RenderCacheEntryReference::logBase2OfCenterSize) - 1));
    util::Vector3I32 maxChunk =
        maxViewLocation
        & ~(util::Vector3I32((1LL << RenderCacheEntryReference::logBase2OfCenterSize) - 1));
    for(util::Vector3I32 chunk = minChunk; chunk.x < maxChunk.x;
        chunk.x += RenderCacheEntryReference::centerSize)
    {
        for(chunk.y = minChunk.y; chunk.y < maxChunk.y;
            chunk.y += RenderCacheEntryReference::centerSize)
        {
            for(chunk.z = minChunk.z; chunk.z < maxChunk.z;
                chunk.z += RenderCacheEntryReference::centerSize)
            {
                auto renderCacheEntry = getRenderCacheEntry(chunk, blockStepGlobalState);
                auto &renderBuffer = std::get<0>(renderCacheEntry);
                if(!renderBuffer)
                {
                    renderBuffer = handleUnrenderedChunk(handleUnrenderedChunkArg,
                                                         std::get<1>(renderCacheEntry));
                    if(renderBuffer)
                        setRenderCacheEntry(std::get<1>(renderCacheEntry), renderBuffer);
                    else
                        continue;
                }
                commandBuffer->appendRenderCommand(
                    renderBuffer,
                    graphics::Transform::translate(static_cast<util::Vector3F>(chunk)
                                                   - viewLocation),
                    viewTransform,
                    projectionTransform);
            }
        }
    }
}
}
}
}
