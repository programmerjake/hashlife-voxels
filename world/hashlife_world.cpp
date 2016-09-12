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
HashlifeWorld::HashlifeWorld(PrivateAccessTag)
    : garbageCollectedHashtable(),
      snapshots(),
      renderCacheEntryReferences(),
      rootNode(garbageCollectedHashtable.getCanonicalEmptyNode(1)),
      renderCache(),
      renderCacheEntryList()
{
}

void HashlifeWorld::collectGarbage(std::size_t garbageCollectTargetNodeCount,
                                   std::size_t renderCacheTargetEntryCount)
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
    std::size_t expectedRootArraysSize = 1 + renderCache.size() + renderCacheEntryReferences.size();
    rootArrays.reserve(expectedRootArraysSize);
    rootArraySizes.reserve(expectedRootArraysSize);
    rootArrays.push_back(treeRoots.data());
    rootArraySizes.push_back(treeRoots.size());
    for(auto *entry : renderCacheEntryList)
    {
        rootArrays.push_back(entry->nodes.data());
        rootArraySizes.push_back(entry->nodes.size());
    }
    for(auto iter = renderCacheEntryReferences.begin(); iter != renderCacheEntryReferences.end();)
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
    garbageCollectedHashtable.garbageCollect(
        rootArrays.data(), rootArraySizes.data(), rootArrays.size(), garbageCollectTargetNodeCount);
}

void HashlifeWorld::expandRoot()
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
                    getAsNonleaf(rootNode)->getChildNode(position);
                newRootNode[position.x][position.y][position.z] =
                    garbageCollectedHashtable.findOrAddNode(HashlifeNonleafNode(newChildNode));
            }
        }
    }
    rootNode = garbageCollectedHashtable.findOrAddNode(HashlifeNonleafNode(newRootNode));
}

const HashlifeNonleafNode::FutureState &HashlifeWorld::getFilledFutureState(
    HashlifeNodeBase *nodeIn, const block::BlockStepGlobalState &stepGlobalState)
{
    constexprAssert(!nodeIn->isLeaf());
    auto node = getAsNonleaf(nodeIn);
    if(node->futureState.node && node->futureState.globalState == stepGlobalState)
    {
        constexprAssert(node->futureState.node->level == node->level - 1);
        return node->futureState;
    }
    HashlifeNonleafNode::FutureState futureState(stepGlobalState);
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
                            for(std::size_t z2 = 0; z2 < blockStepInput.blocks[x2][y2].size(); z2++)
                            {
                                blockStepInput.blocks[x2][y2][z2] =
                                    node->get(util::Vector3I32(x2, y2, z2) + blockStepInputOrigin);
                            }
                        }
                    }
                    auto stepResult = block::BlockDescriptor::step(blockStepInput, stepGlobalState);
                    futureNode[x][y][z] = stepResult.block;
                    futureState.actions[x][y][z] +=
                        std::move(stepResult.extraActions).addOffset(blockStepInputCenter);
                }
            }
        }
        futureState.node = garbageCollectedHashtable.findOrAddNode(HashlifeLeafNode(futureNode));
        constexprAssert(futureState.node->level == node->level - 1);
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
                        for(position.y = 0; position.y < HashlifeNodeBase::levelSize; position.y++)
                        {
                            for(position.z = 0; position.z < HashlifeNodeBase::levelSize;
                                position.z++)
                            {
                                auto inputPosition = chunkPos + position;
                                auto index1 =
                                    inputPosition / util::Vector3I32(HashlifeNodeBase::levelSize);
                                auto index2 =
                                    inputPosition % util::Vector3I32(HashlifeNodeBase::levelSize);
                                auto childNode = node->getChildNode(index1);
                                constexprAssert(childNode->level == node->level - 1);
                                constexprAssert(!childNode->isLeaf());
                                input[position.x][position.y][position.z] =
                                    getAsNonleaf(childNode)->getChildNode(index2);
                                constexprAssert(input[position.x][position.y][position.z]->level
                                                == node->level - 2);
                            }
                        }
                    }
                    auto &result = getFilledFutureState(
                        garbageCollectedHashtable.findOrAddNode(HashlifeNonleafNode(input)),
                        stepGlobalState);
                    constexprAssert(result.node->level == node->level - 2);
                    for(util::Vector3I32 position(0); position.x < HashlifeNodeBase::levelSize;
                        position.x++)
                    {
                        for(position.y = 0; position.y < HashlifeNodeBase::levelSize; position.y++)
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
                                futureState.actions[outputPosition.x][outputPosition
                                                                          .y][outputPosition.z] +=
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
                                    for(position.z = 0; position.z < HashlifeNodeBase::levelSize;
                                        position.z++)
                                    {
                                        auto inputPosition =
                                            chunkPos * util::Vector3I32(HashlifeNodeBase::levelSize)
                                            + position + util::Vector3I32(1);
                                        auto index1 =
                                            inputPosition
                                            / util::Vector3I32(HashlifeNodeBase::levelSize);
                                        auto index2 =
                                            inputPosition
                                            % util::Vector3I32(HashlifeNodeBase::levelSize);
                                        auto childNode = intermediate[index1.x][index1.y][index1.z];
                                        constexprAssert(childNode->isLeaf());
                                        blocks[position.x][position.y][position.z] =
                                            getAsLeaf(childNode)->getBlock(index2);
                                    }
                                }
                            }
                            output[chunkPos.x][chunkPos.y][chunkPos.z] =
                                garbageCollectedHashtable.findOrAddNode(HashlifeLeafNode(blocks));
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
                                    for(position.z = 0; position.z < HashlifeNodeBase::levelSize;
                                        position.z++)
                                    {
                                        auto inputPosition =
                                            chunkPos * util::Vector3I32(HashlifeNodeBase::levelSize)
                                            + position + util::Vector3I32(1);
                                        auto index1 =
                                            inputPosition
                                            / util::Vector3I32(HashlifeNodeBase::levelSize);
                                        auto index2 =
                                            inputPosition
                                            % util::Vector3I32(HashlifeNodeBase::levelSize);
                                        auto childNode = intermediate[index1.x][index1.y][index1.z];
                                        constexprAssert(!childNode->isLeaf());
                                        childNodes[position.x][position.y][position.z] =
                                            getAsNonleaf(childNode)->getChildNode(index2);
                                    }
                                }
                            }
                            output[chunkPos.x][chunkPos.y][chunkPos.z] =
                                garbageCollectedHashtable.findOrAddNode(
                                    HashlifeNonleafNode(childNodes));
                        }
                        constexprAssert(output[chunkPos.x][chunkPos.y][chunkPos.z]->level
                                        == node->level - 2);
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
                                    input[position.x][position.y][position.z] =
                                        intermediate[inputPosition.x][inputPosition.y][inputPosition
                                                                                           .z];
                                }
                            }
                        }
                        auto &result = getFilledFutureState(
                            garbageCollectedHashtable.findOrAddNode(HashlifeNonleafNode(input)),
                            stepGlobalState);
                        constexprAssert(result.node->level == node->level - 2);
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
                                        chunkPos * util::Vector3I32(HashlifeNodeBase::levelSize)
                                        - util::Vector3I32(1);
                                    futureState.actions[chunkPos.x][chunkPos.y][chunkPos.z] +=
                                        block::BlockStepExtraActions(
                                            result.actions[position.x][position.y][position.z])
                                            .addOffset(offsetInEighths
                                                       * util::Vector3I32(node->getEighthSize()));
                                }
                            }
                        }
                        output[chunkPos.x][chunkPos.y][chunkPos.z] = result.node;
                    }
                }
            }
        }
        futureState.node = garbageCollectedHashtable.findOrAddNode(HashlifeNonleafNode(output));
    }
    constexprAssert(futureState.node->level == node->level - 1);
    node->futureState = std::move(futureState);
    return node->futureState;
}

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
                        auto block = getAsLeaf(currentNode)->getBlock(position);
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
                        auto childNode = getAsNonleaf(currentNode)->getChildNode(position);
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
#if 1
#warning renderRenderCacheEntry disabled
    return graphics::EmptyRenderBuffer::get();
#else
    if(!renderCacheEntryReference->getBlockSummary().rendersAnything())
        return graphics::EmptyRenderBuffer::get();
    constexpr std::int32_t blockLightingArraySize =
        RenderCacheEntryReference::centerSize + 2; // one extra block on each side
    constexpr std::int32_t blockArraySize =
        blockLightingArraySize + 2; // one extra block on each side
    struct State final
    {
        util::Array<util::Array<util::Array<lighting::BlockLighting, blockLightingArraySize>,
                                blockLightingArraySize>,
                    blockLightingArraySize> blockLightingArray;
        util::Array<util::Array<util::Array<block::Block, blockArraySize>, blockArraySize>,
                    blockArraySize> blockArray;
    };
    std::unique_ptr<State> state(new State());
    renderCacheEntryReference->getBlocks(state->blockArray,
                                         util::Vector3I32(-2),
                                         util::Vector3I32(0),
                                         util::Vector3I32(blockArraySize));
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
                            auto blockArrayPosition = blockLightingPosition
                                                      + util::Vector3I32(x2, y2, z2)
                                                      + util::Vector3I32(1);
                            auto block =
                                state->blockArray[blockArrayPosition.x][blockArrayPosition.y]
                                                 [blockArrayPosition.z];
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
                auto block = state->blockArray[x + 2][y + 2][z + 2];
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
                            auto blockArrayPosition = util::Vector3I32(x, y, z)
                                                      + util::Vector3I32(x2, y2, z2)
                                                      + util::Vector3I32(1);
                            blockStepInput.blocks[x2][y2][z2] =
                                state->blockArray[blockArrayPosition.x][blockArrayPosition.y]
                                                 [blockArrayPosition.z];
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
#endif
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
    util::FunctionReference<std::shared_ptr<graphics::ReadableRenderBuffer>(
        std::shared_ptr<RenderCacheEntryReference> renderCacheEntryReference)>
        handleUnrenderedChunk,
    util::FunctionReference<void(std::shared_ptr<GPURenderBufferCache::Entry> entry)>
        handleUpdateGPURenderBuffer,
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
                                renderBuffer = handleUnrenderedChunk(std::get<1>(renderCacheEntry));
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
                    handleUpdateGPURenderBuffer(entry);
                if(anyChanges)
                    gpuRenderBufferCache.setEntry(renderBufferPosition, std::move(entry));
            }
        }
    }
}
}
}
}
