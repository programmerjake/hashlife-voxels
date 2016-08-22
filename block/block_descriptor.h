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

#ifndef BLOCK_BLOCK_DESCRIPTOR_H_
#define BLOCK_BLOCK_DESCRIPTOR_H_

#include "block.h"
#include "../lighting/lighting.h"
#include "../util/constexpr_assert.h"
#include "../util/optional.h"
#include "../util/vector.h"
#include "../world/position.h"
#include <array>
#include <functional>
#include <vector>
#include <list>

namespace programmerjake
{
namespace voxels
{
namespace block
{
struct BlockStepGlobalState final
{
    static constexpr std::uint32_t log2OfStepSizeInGenerations = 0;
    static constexpr std::uint32_t stepSizeInGenerations = 1UL << log2OfStepSizeInGenerations;
    lighting::Lighting::GlobalProperties lightingGlobalProperties;
    constexpr world::Dimension getDimension() const
    {
        return lightingGlobalProperties.dimension;
    }
    constexpr bool operator==(const BlockStepGlobalState &rt) const
    {
        return lightingGlobalProperties == rt.lightingGlobalProperties;
    }
    constexpr bool operator!=(const BlockStepGlobalState &rt) const
    {
        return !operator==(rt);
    }
    std::size_t hash() const
    {
        return std::hash<lighting::Lighting::GlobalProperties>()(lightingGlobalProperties);
    }
    constexpr explicit BlockStepGlobalState(
        lighting::Lighting::GlobalProperties lightingGlobalProperties)
        : lightingGlobalProperties(lightingGlobalProperties)
    {
    }
    constexpr BlockStepGlobalState() : lightingGlobalProperties()
    {
    }
};
}
}
}

namespace std
{
template <>
struct hash<programmerjake::voxels::block::BlockStepGlobalState>
{
    std::size_t operator()(programmerjake::voxels::block::BlockStepGlobalState v) const
    {
        return v.hash();
    }
};
}

namespace programmerjake
{
namespace voxels
{
namespace block
{
struct BlockStepInput final
{
    std::array<std::array<std::array<Block, 3>, 3>, 3> blocks;
};

class BlockStepPriority final
{
private:
    const std::ptrdiff_t *priority;
    constexpr explicit BlockStepPriority(const std::ptrdiff_t *priority) : priority(priority)
    {
    }
    std::ptrdiff_t getPriorityValue() const noexcept
    {
        return priority ? *priority : 0;
    }

public:
    constexpr BlockStepPriority() : priority()
    {
    }
    friend bool operator<(BlockStepPriority a, BlockStepPriority b) noexcept
    {
        if(a.priority == b.priority)
            return false;
        return a.getPriorityValue() < b.getPriorityValue();
    }
    friend bool operator>(BlockStepPriority a, BlockStepPriority b) noexcept
    {
        if(a.priority == b.priority)
            return false;
        return a.getPriorityValue() > b.getPriorityValue();
    }
    friend bool operator<=(BlockStepPriority a, BlockStepPriority b) noexcept
    {
        if(a.priority == b.priority)
            return true;
        return a.getPriorityValue() <= b.getPriorityValue();
    }
    friend bool operator>=(BlockStepPriority a, BlockStepPriority b) noexcept
    {
        if(a.priority == b.priority)
            return true;
        return a.getPriorityValue() >= b.getPriorityValue();
    }
    friend bool operator==(BlockStepPriority a, BlockStepPriority b) noexcept
    {
        return a.priority == b.priority;
    }
    friend bool operator!=(BlockStepPriority a, BlockStepPriority b) noexcept
    {
        return a.priority != b.priority;
    }
    std::size_t hash() const noexcept
    {
        return std::hash<const std::ptrdiff_t *>()(priority);
    }
};
}
}
}

namespace std
{
template <>
struct hash<programmerjake::voxels::block::BlockStepPriority>
{
    std::size_t operator()(programmerjake::voxels::block::BlockStepPriority v) const
    {
        return v.hash();
    }
};
}

namespace programmerjake
{
namespace voxels
{
namespace world
{
class World;
}

namespace block
{
struct BlockStepExtraAction final
{
    typedef std::function<void(world::World &theWorld, world::Position3I32 position)>
        ActionFunction;
    ActionFunction actionFunction;
    util::Vector3I32 positionOffset;
    explicit BlockStepExtraAction(ActionFunction actionFunction,
                                  util::Vector3I32 positionOffset = util::Vector3I32(0))
        : actionFunction(std::move(actionFunction)), positionOffset(positionOffset)
    {
    }
    void addOffset(util::Vector3I32 offset)
    {
        positionOffset += offset;
    }
    void run(world::World &theWorld, world::Dimension dimension) const
    {
        actionFunction(theWorld, world::Position3I32(positionOffset, dimension));
    }
};

struct BlockStepExtraActions final
{
    util::Optional<std::list<BlockStepExtraAction>> actions;
    void merge(BlockStepExtraActions newActions)
    {
        if(newActions.actions)
        {
            if(actions)
                actions->splice(actions->end(), std::move(*newActions.actions));
            else
                actions = std::move(newActions.actions);
        }
    }
    bool empty() const
    {
        return !actions;
    }
    constexpr BlockStepExtraActions() : actions()
    {
    }
    explicit BlockStepExtraActions(std::list<BlockStepExtraAction> actions)
        : actions(std::move(actions))
    {
    }
    explicit BlockStepExtraActions(BlockStepExtraAction action) : actions()
    {
        actions.emplace();
        actions->push_back(std::move(action));
    }
    BlockStepExtraActions &addOffset(util::Vector3I32 offset) &
    {
        if(offset != util::Vector3I32(0) && actions)
        {
            for(auto &action : *actions)
                action.addOffset(offset);
        }
        return *this;
    }
    BlockStepExtraActions &&addOffset(util::Vector3I32 offset) &&
    {
        return std::move(addOffset(offset));
    }
    BlockStepExtraActions &operator+=(BlockStepExtraActions rt)
    {
        merge(std::move(rt));
        return *this;
    }
    BlockStepExtraActions operator+(BlockStepExtraActions rt) const &
    {
        return BlockStepExtraActions(*this) += std::move(rt);
    }
    BlockStepExtraActions operator+(BlockStepExtraActions rt) &&
    {
        return BlockStepExtraActions(std::move(*this)) += std::move(rt);
    }
    void run(world::World &theWorld, world::Dimension dimension) const
    {
        if(actions)
        {
            for(auto &action : *actions)
            {
                action.run(theWorld, dimension);
            }
        }
    }
};

struct BlockStepPartOutput final
{
    BlockKind blockKind;
    BlockStepPriority priority;
    BlockStepExtraActions actions;
    bool empty() const
    {
        return blockKind == BlockKind::empty() && actions.empty();
    }
    constexpr BlockStepPartOutput() : blockKind(BlockKind::empty()), priority(), actions()
    {
    }
    BlockStepPartOutput(BlockKind blockKind,
                        BlockStepPriority priority,
                        BlockStepExtraActions actions)
        : blockKind(blockKind), priority(priority), actions(std::move(actions))
    {
    }
    BlockStepPartOutput(BlockKind blockKind,
                        BlockStepPriority priority,
                        std::list<BlockStepExtraAction> actions)
        : blockKind(blockKind), priority(priority), actions(std::move(actions))
    {
    }
    BlockStepPartOutput(BlockKind blockKind,
                        BlockStepPriority priority,
                        BlockStepExtraAction action)
        : blockKind(blockKind), priority(priority), actions(std::move(action))
    {
    }
    constexpr BlockStepPartOutput(BlockKind blockKind, BlockStepPriority priority)
        : blockKind(blockKind), priority(priority), actions()
    {
    }
    BlockStepPartOutput(BlockKind blockKind, BlockStepExtraActions actions)
        : blockKind(blockKind), priority(), actions(std::move(actions))
    {
    }
    BlockStepPartOutput(BlockKind blockKind, std::list<BlockStepExtraAction> actions)
        : blockKind(blockKind), priority(), actions(std::move(actions))
    {
    }
    BlockStepPartOutput(BlockKind blockKind, BlockStepExtraAction action)
        : blockKind(blockKind), priority(), actions(std::move(action))
    {
    }
    constexpr BlockStepPartOutput(BlockKind blockKind) : blockKind(blockKind), priority(), actions()
    {
    }
    BlockStepPartOutput &operator+=(BlockStepPartOutput rt)
    {
        if(rt.empty())
            return *this;
        if(empty())
        {
            *this = std::move(rt);
            return *this;
        }
        actions += std::move(rt.actions);
        if(priority < rt.priority)
        {
            priority = rt.priority;
            blockKind = rt.blockKind;
        }
        return *this;
    }
    BlockStepPartOutput operator+(BlockStepPartOutput rt) const &
    {
        auto retval = *this;
        retval += std::move(rt);
        return retval;
    }
    BlockStepPartOutput operator+(BlockStepPartOutput rt) &&
    {
        auto retval = std::move(*this);
        retval += std::move(rt);
        return retval;
    }
};

struct BlockStepFullOutput final
{
    Block block;
    BlockStepExtraActions extraActions;
    constexpr BlockStepFullOutput() : block(), extraActions()
    {
    }
    BlockStepFullOutput(Block block, BlockStepExtraActions extraActions)
        : block(block), extraActions(std::move(extraActions))
    {
    }
};

class BlockDescriptor
{
    BlockDescriptor(const BlockDescriptor &) = delete;
    BlockDescriptor &operator=(const BlockDescriptor &) = delete;

private:
    static std::vector<const BlockDescriptor *> &getDescriptorsLookupTable() noexcept
    {
        static std::vector<const BlockDescriptor *> *retval =
            new std::vector<const BlockDescriptor *>();
        return *retval;
    }

protected:
    explicit BlockDescriptor(std::string name, lighting::LightProperties lightProperties) noexcept;

public:
    virtual ~BlockDescriptor() = default;
    const BlockKind blockKind;
    const std::string name;
    const lighting::LightProperties lightProperties;
    virtual BlockStepPartOutput stepNXNYNZ(const BlockStepInput &stepInput,
                                           const block::BlockStepGlobalState &stepGlobalState) const
    {
        return BlockStepPartOutput();
    }
    virtual BlockStepPartOutput stepNXNYCZ(const BlockStepInput &stepInput,
                                           const block::BlockStepGlobalState &stepGlobalState) const
    {
        return BlockStepPartOutput();
    }
    virtual BlockStepPartOutput stepNXNYPZ(const BlockStepInput &stepInput,
                                           const block::BlockStepGlobalState &stepGlobalState) const
    {
        return BlockStepPartOutput();
    }
    virtual BlockStepPartOutput stepNXCYNZ(const BlockStepInput &stepInput,
                                           const block::BlockStepGlobalState &stepGlobalState) const
    {
        return BlockStepPartOutput();
    }
    virtual BlockStepPartOutput stepNXCYCZ(const BlockStepInput &stepInput,
                                           const block::BlockStepGlobalState &stepGlobalState) const
    {
        return BlockStepPartOutput();
    }
    virtual BlockStepPartOutput stepNXCYPZ(const BlockStepInput &stepInput,
                                           const block::BlockStepGlobalState &stepGlobalState) const
    {
        return BlockStepPartOutput();
    }
    virtual BlockStepPartOutput stepNXPYNZ(const BlockStepInput &stepInput,
                                           const block::BlockStepGlobalState &stepGlobalState) const
    {
        return BlockStepPartOutput();
    }
    virtual BlockStepPartOutput stepNXPYCZ(const BlockStepInput &stepInput,
                                           const block::BlockStepGlobalState &stepGlobalState) const
    {
        return BlockStepPartOutput();
    }
    virtual BlockStepPartOutput stepNXPYPZ(const BlockStepInput &stepInput,
                                           const block::BlockStepGlobalState &stepGlobalState) const
    {
        return BlockStepPartOutput();
    }
    virtual BlockStepPartOutput stepCXNYNZ(const BlockStepInput &stepInput,
                                           const block::BlockStepGlobalState &stepGlobalState) const
    {
        return BlockStepPartOutput();
    }
    virtual BlockStepPartOutput stepCXNYCZ(const BlockStepInput &stepInput,
                                           const block::BlockStepGlobalState &stepGlobalState) const
    {
        return BlockStepPartOutput();
    }
    virtual BlockStepPartOutput stepCXNYPZ(const BlockStepInput &stepInput,
                                           const block::BlockStepGlobalState &stepGlobalState) const
    {
        return BlockStepPartOutput();
    }
    virtual BlockStepPartOutput stepCXCYNZ(const BlockStepInput &stepInput,
                                           const block::BlockStepGlobalState &stepGlobalState) const
    {
        return BlockStepPartOutput();
    }
    virtual BlockStepPartOutput stepCXCYCZ(const BlockStepInput &stepInput,
                                           const block::BlockStepGlobalState &stepGlobalState) const
    {
        return BlockStepPartOutput();
    }
    virtual BlockStepPartOutput stepCXCYPZ(const BlockStepInput &stepInput,
                                           const block::BlockStepGlobalState &stepGlobalState) const
    {
        return BlockStepPartOutput();
    }
    virtual BlockStepPartOutput stepCXPYNZ(const BlockStepInput &stepInput,
                                           const block::BlockStepGlobalState &stepGlobalState) const
    {
        return BlockStepPartOutput();
    }
    virtual BlockStepPartOutput stepCXPYCZ(const BlockStepInput &stepInput,
                                           const block::BlockStepGlobalState &stepGlobalState) const
    {
        return BlockStepPartOutput();
    }
    virtual BlockStepPartOutput stepCXPYPZ(const BlockStepInput &stepInput,
                                           const block::BlockStepGlobalState &stepGlobalState) const
    {
        return BlockStepPartOutput();
    }
    virtual BlockStepPartOutput stepPXNYNZ(const BlockStepInput &stepInput,
                                           const block::BlockStepGlobalState &stepGlobalState) const
    {
        return BlockStepPartOutput();
    }
    virtual BlockStepPartOutput stepPXNYCZ(const BlockStepInput &stepInput,
                                           const block::BlockStepGlobalState &stepGlobalState) const
    {
        return BlockStepPartOutput();
    }
    virtual BlockStepPartOutput stepPXNYPZ(const BlockStepInput &stepInput,
                                           const block::BlockStepGlobalState &stepGlobalState) const
    {
        return BlockStepPartOutput();
    }
    virtual BlockStepPartOutput stepPXCYNZ(const BlockStepInput &stepInput,
                                           const block::BlockStepGlobalState &stepGlobalState) const
    {
        return BlockStepPartOutput();
    }
    virtual BlockStepPartOutput stepPXCYCZ(const BlockStepInput &stepInput,
                                           const block::BlockStepGlobalState &stepGlobalState) const
    {
        return BlockStepPartOutput();
    }
    virtual BlockStepPartOutput stepPXCYPZ(const BlockStepInput &stepInput,
                                           const block::BlockStepGlobalState &stepGlobalState) const
    {
        return BlockStepPartOutput();
    }
    virtual BlockStepPartOutput stepPXPYNZ(const BlockStepInput &stepInput,
                                           const block::BlockStepGlobalState &stepGlobalState) const
    {
        return BlockStepPartOutput();
    }
    virtual BlockStepPartOutput stepPXPYCZ(const BlockStepInput &stepInput,
                                           const block::BlockStepGlobalState &stepGlobalState) const
    {
        return BlockStepPartOutput();
    }
    virtual BlockStepPartOutput stepPXPYPZ(const BlockStepInput &stepInput,
                                           const block::BlockStepGlobalState &stepGlobalState) const
    {
        return BlockStepPartOutput();
    }

private:
    static BlockStepPartOutput stepNXNYNZ(BlockKind blockKind,
                                          const BlockStepInput &stepInput,
                                          const block::BlockStepGlobalState &stepGlobalState)
    {
        if(blockKind == BlockKind::empty())
            return BlockStepPartOutput();
        return get(blockKind)->stepNXNYNZ(stepInput, stepGlobalState);
    }
    static BlockStepPartOutput stepNXNYCZ(BlockKind blockKind,
                                          const BlockStepInput &stepInput,
                                          const block::BlockStepGlobalState &stepGlobalState)
    {
        if(blockKind == BlockKind::empty())
            return BlockStepPartOutput();
        return get(blockKind)->stepNXNYCZ(stepInput, stepGlobalState);
    }
    static BlockStepPartOutput stepNXNYPZ(BlockKind blockKind,
                                          const BlockStepInput &stepInput,
                                          const block::BlockStepGlobalState &stepGlobalState)
    {
        if(blockKind == BlockKind::empty())
            return BlockStepPartOutput();
        return get(blockKind)->stepNXNYPZ(stepInput, stepGlobalState);
    }
    static BlockStepPartOutput stepNXCYNZ(BlockKind blockKind,
                                          const BlockStepInput &stepInput,
                                          const block::BlockStepGlobalState &stepGlobalState)
    {
        if(blockKind == BlockKind::empty())
            return BlockStepPartOutput();
        return get(blockKind)->stepNXCYNZ(stepInput, stepGlobalState);
    }
    static BlockStepPartOutput stepNXCYCZ(BlockKind blockKind,
                                          const BlockStepInput &stepInput,
                                          const block::BlockStepGlobalState &stepGlobalState)
    {
        if(blockKind == BlockKind::empty())
            return BlockStepPartOutput();
        return get(blockKind)->stepNXCYCZ(stepInput, stepGlobalState);
    }
    static BlockStepPartOutput stepNXCYPZ(BlockKind blockKind,
                                          const BlockStepInput &stepInput,
                                          const block::BlockStepGlobalState &stepGlobalState)
    {
        if(blockKind == BlockKind::empty())
            return BlockStepPartOutput();
        return get(blockKind)->stepNXCYPZ(stepInput, stepGlobalState);
    }
    static BlockStepPartOutput stepNXPYNZ(BlockKind blockKind,
                                          const BlockStepInput &stepInput,
                                          const block::BlockStepGlobalState &stepGlobalState)
    {
        if(blockKind == BlockKind::empty())
            return BlockStepPartOutput();
        return get(blockKind)->stepNXPYNZ(stepInput, stepGlobalState);
    }
    static BlockStepPartOutput stepNXPYCZ(BlockKind blockKind,
                                          const BlockStepInput &stepInput,
                                          const block::BlockStepGlobalState &stepGlobalState)
    {
        if(blockKind == BlockKind::empty())
            return BlockStepPartOutput();
        return get(blockKind)->stepNXPYCZ(stepInput, stepGlobalState);
    }
    static BlockStepPartOutput stepNXPYPZ(BlockKind blockKind,
                                          const BlockStepInput &stepInput,
                                          const block::BlockStepGlobalState &stepGlobalState)
    {
        if(blockKind == BlockKind::empty())
            return BlockStepPartOutput();
        return get(blockKind)->stepNXPYPZ(stepInput, stepGlobalState);
    }
    static BlockStepPartOutput stepCXNYNZ(BlockKind blockKind,
                                          const BlockStepInput &stepInput,
                                          const block::BlockStepGlobalState &stepGlobalState)
    {
        if(blockKind == BlockKind::empty())
            return BlockStepPartOutput();
        return get(blockKind)->stepCXNYNZ(stepInput, stepGlobalState);
    }
    static BlockStepPartOutput stepCXNYCZ(BlockKind blockKind,
                                          const BlockStepInput &stepInput,
                                          const block::BlockStepGlobalState &stepGlobalState)
    {
        if(blockKind == BlockKind::empty())
            return BlockStepPartOutput();
        return get(blockKind)->stepCXNYCZ(stepInput, stepGlobalState);
    }
    static BlockStepPartOutput stepCXNYPZ(BlockKind blockKind,
                                          const BlockStepInput &stepInput,
                                          const block::BlockStepGlobalState &stepGlobalState)
    {
        if(blockKind == BlockKind::empty())
            return BlockStepPartOutput();
        return get(blockKind)->stepCXNYPZ(stepInput, stepGlobalState);
    }
    static BlockStepPartOutput stepCXCYNZ(BlockKind blockKind,
                                          const BlockStepInput &stepInput,
                                          const block::BlockStepGlobalState &stepGlobalState)
    {
        if(blockKind == BlockKind::empty())
            return BlockStepPartOutput();
        return get(blockKind)->stepCXCYNZ(stepInput, stepGlobalState);
    }
    static BlockStepPartOutput stepCXCYCZ(BlockKind blockKind,
                                          const BlockStepInput &stepInput,
                                          const block::BlockStepGlobalState &stepGlobalState)
    {
        if(blockKind == BlockKind::empty())
            return BlockStepPartOutput();
        return get(blockKind)->stepCXCYCZ(stepInput, stepGlobalState);
    }
    static BlockStepPartOutput stepCXCYPZ(BlockKind blockKind,
                                          const BlockStepInput &stepInput,
                                          const block::BlockStepGlobalState &stepGlobalState)
    {
        if(blockKind == BlockKind::empty())
            return BlockStepPartOutput();
        return get(blockKind)->stepCXCYPZ(stepInput, stepGlobalState);
    }
    static BlockStepPartOutput stepCXPYNZ(BlockKind blockKind,
                                          const BlockStepInput &stepInput,
                                          const block::BlockStepGlobalState &stepGlobalState)
    {
        if(blockKind == BlockKind::empty())
            return BlockStepPartOutput();
        return get(blockKind)->stepCXPYNZ(stepInput, stepGlobalState);
    }
    static BlockStepPartOutput stepCXPYCZ(BlockKind blockKind,
                                          const BlockStepInput &stepInput,
                                          const block::BlockStepGlobalState &stepGlobalState)
    {
        if(blockKind == BlockKind::empty())
            return BlockStepPartOutput();
        return get(blockKind)->stepCXPYCZ(stepInput, stepGlobalState);
    }
    static BlockStepPartOutput stepCXPYPZ(BlockKind blockKind,
                                          const BlockStepInput &stepInput,
                                          const block::BlockStepGlobalState &stepGlobalState)
    {
        if(blockKind == BlockKind::empty())
            return BlockStepPartOutput();
        return get(blockKind)->stepCXPYPZ(stepInput, stepGlobalState);
    }
    static BlockStepPartOutput stepPXNYNZ(BlockKind blockKind,
                                          const BlockStepInput &stepInput,
                                          const block::BlockStepGlobalState &stepGlobalState)
    {
        if(blockKind == BlockKind::empty())
            return BlockStepPartOutput();
        return get(blockKind)->stepPXNYNZ(stepInput, stepGlobalState);
    }
    static BlockStepPartOutput stepPXNYCZ(BlockKind blockKind,
                                          const BlockStepInput &stepInput,
                                          const block::BlockStepGlobalState &stepGlobalState)
    {
        if(blockKind == BlockKind::empty())
            return BlockStepPartOutput();
        return get(blockKind)->stepPXNYCZ(stepInput, stepGlobalState);
    }
    static BlockStepPartOutput stepPXNYPZ(BlockKind blockKind,
                                          const BlockStepInput &stepInput,
                                          const block::BlockStepGlobalState &stepGlobalState)
    {
        if(blockKind == BlockKind::empty())
            return BlockStepPartOutput();
        return get(blockKind)->stepPXNYPZ(stepInput, stepGlobalState);
    }
    static BlockStepPartOutput stepPXCYNZ(BlockKind blockKind,
                                          const BlockStepInput &stepInput,
                                          const block::BlockStepGlobalState &stepGlobalState)
    {
        if(blockKind == BlockKind::empty())
            return BlockStepPartOutput();
        return get(blockKind)->stepPXCYNZ(stepInput, stepGlobalState);
    }
    static BlockStepPartOutput stepPXCYCZ(BlockKind blockKind,
                                          const BlockStepInput &stepInput,
                                          const block::BlockStepGlobalState &stepGlobalState)
    {
        if(blockKind == BlockKind::empty())
            return BlockStepPartOutput();
        return get(blockKind)->stepPXCYCZ(stepInput, stepGlobalState);
    }
    static BlockStepPartOutput stepPXCYPZ(BlockKind blockKind,
                                          const BlockStepInput &stepInput,
                                          const block::BlockStepGlobalState &stepGlobalState)
    {
        if(blockKind == BlockKind::empty())
            return BlockStepPartOutput();
        return get(blockKind)->stepPXCYPZ(stepInput, stepGlobalState);
    }
    static BlockStepPartOutput stepPXPYNZ(BlockKind blockKind,
                                          const BlockStepInput &stepInput,
                                          const block::BlockStepGlobalState &stepGlobalState)
    {
        if(blockKind == BlockKind::empty())
            return BlockStepPartOutput();
        return get(blockKind)->stepPXPYNZ(stepInput, stepGlobalState);
    }
    static BlockStepPartOutput stepPXPYCZ(BlockKind blockKind,
                                          const BlockStepInput &stepInput,
                                          const block::BlockStepGlobalState &stepGlobalState)
    {
        if(blockKind == BlockKind::empty())
            return BlockStepPartOutput();
        return get(blockKind)->stepPXPYCZ(stepInput, stepGlobalState);
    }
    static BlockStepPartOutput stepPXPYPZ(BlockKind blockKind,
                                          const BlockStepInput &stepInput,
                                          const block::BlockStepGlobalState &stepGlobalState)
    {
        if(blockKind == BlockKind::empty())
            return BlockStepPartOutput();
        return get(blockKind)->stepPXPYPZ(stepInput, stepGlobalState);
    }

public:
    static const BlockDescriptor *get(BlockKind blockKind) noexcept
    {
        static_assert(BlockKind::empty().value == 0, "");
        if(blockKind == BlockKind::empty())
            return nullptr;
        blockKind.value--;
        auto &descriptorsLookupTable = getDescriptorsLookupTable();
        constexprAssert(blockKind.value < descriptorsLookupTable.size());
        return descriptorsLookupTable[blockKind.value];
    }
    static BlockStepFullOutput step(const BlockStepInput &stepInput,
                                    const block::BlockStepGlobalState &stepGlobalState)
    {
        if(stepInput.blocks[1][1][1].getBlockKind() == BlockKind::empty())
            return BlockStepFullOutput(stepInput.blocks[1][1][1], BlockStepExtraActions());
        BlockStepPartOutput blockStepPartOutput =
            stepNXNYNZ(stepInput.blocks[0][0][0].getBlockKind(), stepInput, stepGlobalState);
        blockStepPartOutput +=
            stepNXNYCZ(stepInput.blocks[0][0][1].getBlockKind(), stepInput, stepGlobalState);
        blockStepPartOutput +=
            stepNXNYPZ(stepInput.blocks[0][0][2].getBlockKind(), stepInput, stepGlobalState);
        blockStepPartOutput +=
            stepNXCYNZ(stepInput.blocks[0][1][0].getBlockKind(), stepInput, stepGlobalState);
        blockStepPartOutput +=
            stepNXCYCZ(stepInput.blocks[0][1][1].getBlockKind(), stepInput, stepGlobalState);
        blockStepPartOutput +=
            stepNXCYPZ(stepInput.blocks[0][1][2].getBlockKind(), stepInput, stepGlobalState);
        blockStepPartOutput +=
            stepNXPYNZ(stepInput.blocks[0][2][0].getBlockKind(), stepInput, stepGlobalState);
        blockStepPartOutput +=
            stepNXPYCZ(stepInput.blocks[0][2][1].getBlockKind(), stepInput, stepGlobalState);
        blockStepPartOutput +=
            stepNXPYPZ(stepInput.blocks[0][2][2].getBlockKind(), stepInput, stepGlobalState);
        blockStepPartOutput +=
            stepCXNYNZ(stepInput.blocks[1][0][0].getBlockKind(), stepInput, stepGlobalState);
        blockStepPartOutput +=
            stepCXNYCZ(stepInput.blocks[1][0][1].getBlockKind(), stepInput, stepGlobalState);
        blockStepPartOutput +=
            stepCXNYPZ(stepInput.blocks[1][0][2].getBlockKind(), stepInput, stepGlobalState);
        blockStepPartOutput +=
            stepCXCYNZ(stepInput.blocks[1][1][0].getBlockKind(), stepInput, stepGlobalState);
        blockStepPartOutput +=
            stepCXCYCZ(stepInput.blocks[1][1][1].getBlockKind(), stepInput, stepGlobalState);
        blockStepPartOutput +=
            stepCXCYPZ(stepInput.blocks[1][1][2].getBlockKind(), stepInput, stepGlobalState);
        blockStepPartOutput +=
            stepCXPYNZ(stepInput.blocks[1][2][0].getBlockKind(), stepInput, stepGlobalState);
        blockStepPartOutput +=
            stepCXPYCZ(stepInput.blocks[1][2][1].getBlockKind(), stepInput, stepGlobalState);
        blockStepPartOutput +=
            stepCXPYPZ(stepInput.blocks[1][2][2].getBlockKind(), stepInput, stepGlobalState);
        blockStepPartOutput +=
            stepPXNYNZ(stepInput.blocks[2][0][0].getBlockKind(), stepInput, stepGlobalState);
        blockStepPartOutput +=
            stepPXNYCZ(stepInput.blocks[2][0][1].getBlockKind(), stepInput, stepGlobalState);
        blockStepPartOutput +=
            stepPXNYPZ(stepInput.blocks[2][0][2].getBlockKind(), stepInput, stepGlobalState);
        blockStepPartOutput +=
            stepPXCYNZ(stepInput.blocks[2][1][0].getBlockKind(), stepInput, stepGlobalState);
        blockStepPartOutput +=
            stepPXCYCZ(stepInput.blocks[2][1][1].getBlockKind(), stepInput, stepGlobalState);
        blockStepPartOutput +=
            stepPXCYPZ(stepInput.blocks[2][1][2].getBlockKind(), stepInput, stepGlobalState);
        blockStepPartOutput +=
            stepPXPYNZ(stepInput.blocks[2][2][0].getBlockKind(), stepInput, stepGlobalState);
        blockStepPartOutput +=
            stepPXPYCZ(stepInput.blocks[2][2][1].getBlockKind(), stepInput, stepGlobalState);
        blockStepPartOutput +=
            stepPXPYPZ(stepInput.blocks[2][2][2].getBlockKind(), stepInput, stepGlobalState);
        BlockKind outputBlockKind = blockStepPartOutput.blockKind == BlockKind::empty() ?
                                        stepInput.blocks[1][1][1].getBlockKind() :
                                        blockStepPartOutput.blockKind;
        return BlockStepFullOutput(
            Block(outputBlockKind,
                  get(outputBlockKind)
                      ->lightProperties.eval(stepInput.blocks[0][1][1].getLighting(),
                                             stepInput.blocks[2][1][1].getLighting(),
                                             stepInput.blocks[1][0][1].getLighting(),
                                             stepInput.blocks[1][2][1].getLighting(),
                                             stepInput.blocks[1][1][0].getLighting(),
                                             stepInput.blocks[1][1][2].getLighting())),
            std::move(blockStepPartOutput.actions));
    }
};
}
}
}

#endif /* BLOCK_BLOCK_DESCRIPTOR_H_ */
