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
#include "../util/constexpr_array.h"
#include "../graphics/color.h"
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
    static constexpr std::uint32_t log2OfStepSizeInGenerations = 5;
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
    util::Array<util::Array<util::Array<Block, 3>, 3>, 3> blocks;
    Block &operator[](util::Vector3I32 index) noexcept
    {
        return blocks[index.x + 1][index.y + 1][index.z + 1];
    }
    constexpr const Block &operator[](util::Vector3I32 index) const noexcept
    {
        return blocks[index.x + 1][index.y + 1][index.z + 1];
    }
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
    std::unique_ptr<std::list<BlockStepExtraAction>> actions;
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
        : actions(new std::list<BlockStepExtraAction>(std::move(actions)))
    {
    }
    explicit BlockStepExtraActions(BlockStepExtraAction action) : actions()
    {
        actions.reset(new std::list<BlockStepExtraAction>);
        actions->push_back(std::move(action));
    }
    BlockStepExtraActions &operator=(BlockStepExtraActions rt)
    {
        actions = std::move(rt.actions);
        return *this;
    }
    BlockStepExtraActions(BlockStepExtraActions &&) = default;
    BlockStepExtraActions(const BlockStepExtraActions &rt) : actions()
    {
        if(rt.actions)
        {
            actions.reset(new std::list<BlockStepExtraAction>(*rt.actions));
        }
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
}

namespace graphics
{
class MemoryRenderBuffer;
struct Transform;
}

namespace block
{
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

public:
    typedef util::EnumArray<bool, BlockFace> BlockedFaces;

protected:
    explicit BlockDescriptor(std::string name,
                             lighting::LightProperties lightProperties,
                             const BlockedFaces &blockedFaces,
                             const BlockSummary &blockSummary) noexcept;

public:
    virtual ~BlockDescriptor() = default;
    const lighting::LightProperties lightProperties;
    const BlockKind blockKind;
    const std::string name;
    const BlockedFaces blockedFaces;
    const BlockSummary blockSummary;
    virtual void render(
        graphics::MemoryRenderBuffer &renderBuffer,
        const BlockStepInput &stepInput,
        const BlockStepGlobalState &stepGlobalState,
        const util::EnumArray<const lighting::BlockLighting *, BlockFace> &blockLightingForFaces,
        const lighting::BlockLighting &blockLightingForCenter,
        const graphics::Transform &transform) const = 0;
    virtual BlockStepPartOutput stepFromNXNYNZ(const BlockStepInput &stepInput,
                                               const BlockStepGlobalState &stepGlobalState) const
    {
        return BlockStepPartOutput();
    }
    virtual BlockStepPartOutput stepFromNXNYCZ(const BlockStepInput &stepInput,
                                               const BlockStepGlobalState &stepGlobalState) const
    {
        return BlockStepPartOutput();
    }
    virtual BlockStepPartOutput stepFromNXNYPZ(const BlockStepInput &stepInput,
                                               const BlockStepGlobalState &stepGlobalState) const
    {
        return BlockStepPartOutput();
    }
    virtual BlockStepPartOutput stepFromNXCYNZ(const BlockStepInput &stepInput,
                                               const BlockStepGlobalState &stepGlobalState) const
    {
        return BlockStepPartOutput();
    }
    virtual BlockStepPartOutput stepFromNXCYCZ(const BlockStepInput &stepInput,
                                               const BlockStepGlobalState &stepGlobalState) const
    {
        return BlockStepPartOutput();
    }
    virtual BlockStepPartOutput stepFromNXCYPZ(const BlockStepInput &stepInput,
                                               const BlockStepGlobalState &stepGlobalState) const
    {
        return BlockStepPartOutput();
    }
    virtual BlockStepPartOutput stepFromNXPYNZ(const BlockStepInput &stepInput,
                                               const BlockStepGlobalState &stepGlobalState) const
    {
        return BlockStepPartOutput();
    }
    virtual BlockStepPartOutput stepFromNXPYCZ(const BlockStepInput &stepInput,
                                               const BlockStepGlobalState &stepGlobalState) const
    {
        return BlockStepPartOutput();
    }
    virtual BlockStepPartOutput stepFromNXPYPZ(const BlockStepInput &stepInput,
                                               const BlockStepGlobalState &stepGlobalState) const
    {
        return BlockStepPartOutput();
    }
    virtual BlockStepPartOutput stepFromCXNYNZ(const BlockStepInput &stepInput,
                                               const BlockStepGlobalState &stepGlobalState) const
    {
        return BlockStepPartOutput();
    }
    virtual BlockStepPartOutput stepFromCXNYCZ(const BlockStepInput &stepInput,
                                               const BlockStepGlobalState &stepGlobalState) const
    {
        return BlockStepPartOutput();
    }
    virtual BlockStepPartOutput stepFromCXNYPZ(const BlockStepInput &stepInput,
                                               const BlockStepGlobalState &stepGlobalState) const
    {
        return BlockStepPartOutput();
    }
    virtual BlockStepPartOutput stepFromCXCYNZ(const BlockStepInput &stepInput,
                                               const BlockStepGlobalState &stepGlobalState) const
    {
        return BlockStepPartOutput();
    }
    virtual BlockStepPartOutput stepFromCXCYCZ(const BlockStepInput &stepInput,
                                               const BlockStepGlobalState &stepGlobalState) const
    {
        return BlockStepPartOutput();
    }
    virtual BlockStepPartOutput stepFromCXCYPZ(const BlockStepInput &stepInput,
                                               const BlockStepGlobalState &stepGlobalState) const
    {
        return BlockStepPartOutput();
    }
    virtual BlockStepPartOutput stepFromCXPYNZ(const BlockStepInput &stepInput,
                                               const BlockStepGlobalState &stepGlobalState) const
    {
        return BlockStepPartOutput();
    }
    virtual BlockStepPartOutput stepFromCXPYCZ(const BlockStepInput &stepInput,
                                               const BlockStepGlobalState &stepGlobalState) const
    {
        return BlockStepPartOutput();
    }
    virtual BlockStepPartOutput stepFromCXPYPZ(const BlockStepInput &stepInput,
                                               const BlockStepGlobalState &stepGlobalState) const
    {
        return BlockStepPartOutput();
    }
    virtual BlockStepPartOutput stepFromPXNYNZ(const BlockStepInput &stepInput,
                                               const BlockStepGlobalState &stepGlobalState) const
    {
        return BlockStepPartOutput();
    }
    virtual BlockStepPartOutput stepFromPXNYCZ(const BlockStepInput &stepInput,
                                               const BlockStepGlobalState &stepGlobalState) const
    {
        return BlockStepPartOutput();
    }
    virtual BlockStepPartOutput stepFromPXNYPZ(const BlockStepInput &stepInput,
                                               const BlockStepGlobalState &stepGlobalState) const
    {
        return BlockStepPartOutput();
    }
    virtual BlockStepPartOutput stepFromPXCYNZ(const BlockStepInput &stepInput,
                                               const BlockStepGlobalState &stepGlobalState) const
    {
        return BlockStepPartOutput();
    }
    virtual BlockStepPartOutput stepFromPXCYCZ(const BlockStepInput &stepInput,
                                               const BlockStepGlobalState &stepGlobalState) const
    {
        return BlockStepPartOutput();
    }
    virtual BlockStepPartOutput stepFromPXCYPZ(const BlockStepInput &stepInput,
                                               const BlockStepGlobalState &stepGlobalState) const
    {
        return BlockStepPartOutput();
    }
    virtual BlockStepPartOutput stepFromPXPYNZ(const BlockStepInput &stepInput,
                                               const BlockStepGlobalState &stepGlobalState) const
    {
        return BlockStepPartOutput();
    }
    virtual BlockStepPartOutput stepFromPXPYCZ(const BlockStepInput &stepInput,
                                               const BlockStepGlobalState &stepGlobalState) const
    {
        return BlockStepPartOutput();
    }
    virtual BlockStepPartOutput stepFromPXPYPZ(const BlockStepInput &stepInput,
                                               const BlockStepGlobalState &stepGlobalState) const
    {
        return BlockStepPartOutput();
    }

private:
    static BlockStepPartOutput stepFromNXNYNZ(BlockKind blockKind,
                                              const BlockStepInput &stepInput,
                                              const BlockStepGlobalState &stepGlobalState)
    {
        if(blockKind == BlockKind::empty())
            return BlockStepPartOutput();
        return get(blockKind)->stepFromNXNYNZ(stepInput, stepGlobalState);
    }
    static BlockStepPartOutput stepFromNXNYCZ(BlockKind blockKind,
                                              const BlockStepInput &stepInput,
                                              const BlockStepGlobalState &stepGlobalState)
    {
        if(blockKind == BlockKind::empty())
            return BlockStepPartOutput();
        return get(blockKind)->stepFromNXNYCZ(stepInput, stepGlobalState);
    }
    static BlockStepPartOutput stepFromNXNYPZ(BlockKind blockKind,
                                              const BlockStepInput &stepInput,
                                              const BlockStepGlobalState &stepGlobalState)
    {
        if(blockKind == BlockKind::empty())
            return BlockStepPartOutput();
        return get(blockKind)->stepFromNXNYPZ(stepInput, stepGlobalState);
    }
    static BlockStepPartOutput stepFromNXCYNZ(BlockKind blockKind,
                                              const BlockStepInput &stepInput,
                                              const BlockStepGlobalState &stepGlobalState)
    {
        if(blockKind == BlockKind::empty())
            return BlockStepPartOutput();
        return get(blockKind)->stepFromNXCYNZ(stepInput, stepGlobalState);
    }
    static BlockStepPartOutput stepFromNXCYCZ(BlockKind blockKind,
                                              const BlockStepInput &stepInput,
                                              const BlockStepGlobalState &stepGlobalState)
    {
        if(blockKind == BlockKind::empty())
            return BlockStepPartOutput();
        return get(blockKind)->stepFromNXCYCZ(stepInput, stepGlobalState);
    }
    static BlockStepPartOutput stepFromNXCYPZ(BlockKind blockKind,
                                              const BlockStepInput &stepInput,
                                              const BlockStepGlobalState &stepGlobalState)
    {
        if(blockKind == BlockKind::empty())
            return BlockStepPartOutput();
        return get(blockKind)->stepFromNXCYPZ(stepInput, stepGlobalState);
    }
    static BlockStepPartOutput stepFromNXPYNZ(BlockKind blockKind,
                                              const BlockStepInput &stepInput,
                                              const BlockStepGlobalState &stepGlobalState)
    {
        if(blockKind == BlockKind::empty())
            return BlockStepPartOutput();
        return get(blockKind)->stepFromNXPYNZ(stepInput, stepGlobalState);
    }
    static BlockStepPartOutput stepFromNXPYCZ(BlockKind blockKind,
                                              const BlockStepInput &stepInput,
                                              const BlockStepGlobalState &stepGlobalState)
    {
        if(blockKind == BlockKind::empty())
            return BlockStepPartOutput();
        return get(blockKind)->stepFromNXPYCZ(stepInput, stepGlobalState);
    }
    static BlockStepPartOutput stepFromNXPYPZ(BlockKind blockKind,
                                              const BlockStepInput &stepInput,
                                              const BlockStepGlobalState &stepGlobalState)
    {
        if(blockKind == BlockKind::empty())
            return BlockStepPartOutput();
        return get(blockKind)->stepFromNXPYPZ(stepInput, stepGlobalState);
    }
    static BlockStepPartOutput stepFromCXNYNZ(BlockKind blockKind,
                                              const BlockStepInput &stepInput,
                                              const BlockStepGlobalState &stepGlobalState)
    {
        if(blockKind == BlockKind::empty())
            return BlockStepPartOutput();
        return get(blockKind)->stepFromCXNYNZ(stepInput, stepGlobalState);
    }
    static BlockStepPartOutput stepFromCXNYCZ(BlockKind blockKind,
                                              const BlockStepInput &stepInput,
                                              const BlockStepGlobalState &stepGlobalState)
    {
        if(blockKind == BlockKind::empty())
            return BlockStepPartOutput();
        return get(blockKind)->stepFromCXNYCZ(stepInput, stepGlobalState);
    }
    static BlockStepPartOutput stepFromCXNYPZ(BlockKind blockKind,
                                              const BlockStepInput &stepInput,
                                              const BlockStepGlobalState &stepGlobalState)
    {
        if(blockKind == BlockKind::empty())
            return BlockStepPartOutput();
        return get(blockKind)->stepFromCXNYPZ(stepInput, stepGlobalState);
    }
    static BlockStepPartOutput stepFromCXCYNZ(BlockKind blockKind,
                                              const BlockStepInput &stepInput,
                                              const BlockStepGlobalState &stepGlobalState)
    {
        if(blockKind == BlockKind::empty())
            return BlockStepPartOutput();
        return get(blockKind)->stepFromCXCYNZ(stepInput, stepGlobalState);
    }
    static BlockStepPartOutput stepFromCXCYCZ(BlockKind blockKind,
                                              const BlockStepInput &stepInput,
                                              const BlockStepGlobalState &stepGlobalState)
    {
        if(blockKind == BlockKind::empty())
            return BlockStepPartOutput();
        return get(blockKind)->stepFromCXCYCZ(stepInput, stepGlobalState);
    }
    static BlockStepPartOutput stepFromCXCYPZ(BlockKind blockKind,
                                              const BlockStepInput &stepInput,
                                              const BlockStepGlobalState &stepGlobalState)
    {
        if(blockKind == BlockKind::empty())
            return BlockStepPartOutput();
        return get(blockKind)->stepFromCXCYPZ(stepInput, stepGlobalState);
    }
    static BlockStepPartOutput stepFromCXPYNZ(BlockKind blockKind,
                                              const BlockStepInput &stepInput,
                                              const BlockStepGlobalState &stepGlobalState)
    {
        if(blockKind == BlockKind::empty())
            return BlockStepPartOutput();
        return get(blockKind)->stepFromCXPYNZ(stepInput, stepGlobalState);
    }
    static BlockStepPartOutput stepFromCXPYCZ(BlockKind blockKind,
                                              const BlockStepInput &stepInput,
                                              const BlockStepGlobalState &stepGlobalState)
    {
        if(blockKind == BlockKind::empty())
            return BlockStepPartOutput();
        return get(blockKind)->stepFromCXPYCZ(stepInput, stepGlobalState);
    }
    static BlockStepPartOutput stepFromCXPYPZ(BlockKind blockKind,
                                              const BlockStepInput &stepInput,
                                              const BlockStepGlobalState &stepGlobalState)
    {
        if(blockKind == BlockKind::empty())
            return BlockStepPartOutput();
        return get(blockKind)->stepFromCXPYPZ(stepInput, stepGlobalState);
    }
    static BlockStepPartOutput stepFromPXNYNZ(BlockKind blockKind,
                                              const BlockStepInput &stepInput,
                                              const BlockStepGlobalState &stepGlobalState)
    {
        if(blockKind == BlockKind::empty())
            return BlockStepPartOutput();
        return get(blockKind)->stepFromPXNYNZ(stepInput, stepGlobalState);
    }
    static BlockStepPartOutput stepFromPXNYCZ(BlockKind blockKind,
                                              const BlockStepInput &stepInput,
                                              const BlockStepGlobalState &stepGlobalState)
    {
        if(blockKind == BlockKind::empty())
            return BlockStepPartOutput();
        return get(blockKind)->stepFromPXNYCZ(stepInput, stepGlobalState);
    }
    static BlockStepPartOutput stepFromPXNYPZ(BlockKind blockKind,
                                              const BlockStepInput &stepInput,
                                              const BlockStepGlobalState &stepGlobalState)
    {
        if(blockKind == BlockKind::empty())
            return BlockStepPartOutput();
        return get(blockKind)->stepFromPXNYPZ(stepInput, stepGlobalState);
    }
    static BlockStepPartOutput stepFromPXCYNZ(BlockKind blockKind,
                                              const BlockStepInput &stepInput,
                                              const BlockStepGlobalState &stepGlobalState)
    {
        if(blockKind == BlockKind::empty())
            return BlockStepPartOutput();
        return get(blockKind)->stepFromPXCYNZ(stepInput, stepGlobalState);
    }
    static BlockStepPartOutput stepFromPXCYCZ(BlockKind blockKind,
                                              const BlockStepInput &stepInput,
                                              const BlockStepGlobalState &stepGlobalState)
    {
        if(blockKind == BlockKind::empty())
            return BlockStepPartOutput();
        return get(blockKind)->stepFromPXCYCZ(stepInput, stepGlobalState);
    }
    static BlockStepPartOutput stepFromPXCYPZ(BlockKind blockKind,
                                              const BlockStepInput &stepInput,
                                              const BlockStepGlobalState &stepGlobalState)
    {
        if(blockKind == BlockKind::empty())
            return BlockStepPartOutput();
        return get(blockKind)->stepFromPXCYPZ(stepInput, stepGlobalState);
    }
    static BlockStepPartOutput stepFromPXPYNZ(BlockKind blockKind,
                                              const BlockStepInput &stepInput,
                                              const BlockStepGlobalState &stepGlobalState)
    {
        if(blockKind == BlockKind::empty())
            return BlockStepPartOutput();
        return get(blockKind)->stepFromPXPYNZ(stepInput, stepGlobalState);
    }
    static BlockStepPartOutput stepFromPXPYCZ(BlockKind blockKind,
                                              const BlockStepInput &stepInput,
                                              const BlockStepGlobalState &stepGlobalState)
    {
        if(blockKind == BlockKind::empty())
            return BlockStepPartOutput();
        return get(blockKind)->stepFromPXPYCZ(stepInput, stepGlobalState);
    }
    static BlockStepPartOutput stepFromPXPYPZ(BlockKind blockKind,
                                              const BlockStepInput &stepInput,
                                              const BlockStepGlobalState &stepGlobalState)
    {
        if(blockKind == BlockKind::empty())
            return BlockStepPartOutput();
        return get(blockKind)->stepFromPXPYPZ(stepInput, stepGlobalState);
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
                                    const BlockStepGlobalState &stepGlobalState)
    {
        if(stepInput.blocks[1][1][1].getBlockKind() == BlockKind::empty())
            return BlockStepFullOutput(stepInput.blocks[1][1][1], BlockStepExtraActions());
        BlockStepPartOutput blockStepPartOutput =
            stepFromNXNYNZ(stepInput.blocks[0][0][0].getBlockKind(), stepInput, stepGlobalState);
        blockStepPartOutput +=
            stepFromNXNYCZ(stepInput.blocks[0][0][1].getBlockKind(), stepInput, stepGlobalState);
        blockStepPartOutput +=
            stepFromNXNYPZ(stepInput.blocks[0][0][2].getBlockKind(), stepInput, stepGlobalState);
        blockStepPartOutput +=
            stepFromNXCYNZ(stepInput.blocks[0][1][0].getBlockKind(), stepInput, stepGlobalState);
        blockStepPartOutput +=
            stepFromNXCYCZ(stepInput.blocks[0][1][1].getBlockKind(), stepInput, stepGlobalState);
        blockStepPartOutput +=
            stepFromNXCYPZ(stepInput.blocks[0][1][2].getBlockKind(), stepInput, stepGlobalState);
        blockStepPartOutput +=
            stepFromNXPYNZ(stepInput.blocks[0][2][0].getBlockKind(), stepInput, stepGlobalState);
        blockStepPartOutput +=
            stepFromNXPYCZ(stepInput.blocks[0][2][1].getBlockKind(), stepInput, stepGlobalState);
        blockStepPartOutput +=
            stepFromNXPYPZ(stepInput.blocks[0][2][2].getBlockKind(), stepInput, stepGlobalState);
        blockStepPartOutput +=
            stepFromCXNYNZ(stepInput.blocks[1][0][0].getBlockKind(), stepInput, stepGlobalState);
        blockStepPartOutput +=
            stepFromCXNYCZ(stepInput.blocks[1][0][1].getBlockKind(), stepInput, stepGlobalState);
        blockStepPartOutput +=
            stepFromCXNYPZ(stepInput.blocks[1][0][2].getBlockKind(), stepInput, stepGlobalState);
        blockStepPartOutput +=
            stepFromCXCYNZ(stepInput.blocks[1][1][0].getBlockKind(), stepInput, stepGlobalState);
        blockStepPartOutput +=
            stepFromCXCYCZ(stepInput.blocks[1][1][1].getBlockKind(), stepInput, stepGlobalState);
        blockStepPartOutput +=
            stepFromCXCYPZ(stepInput.blocks[1][1][2].getBlockKind(), stepInput, stepGlobalState);
        blockStepPartOutput +=
            stepFromCXPYNZ(stepInput.blocks[1][2][0].getBlockKind(), stepInput, stepGlobalState);
        blockStepPartOutput +=
            stepFromCXPYCZ(stepInput.blocks[1][2][1].getBlockKind(), stepInput, stepGlobalState);
        blockStepPartOutput +=
            stepFromCXPYPZ(stepInput.blocks[1][2][2].getBlockKind(), stepInput, stepGlobalState);
        blockStepPartOutput +=
            stepFromPXNYNZ(stepInput.blocks[2][0][0].getBlockKind(), stepInput, stepGlobalState);
        blockStepPartOutput +=
            stepFromPXNYCZ(stepInput.blocks[2][0][1].getBlockKind(), stepInput, stepGlobalState);
        blockStepPartOutput +=
            stepFromPXNYPZ(stepInput.blocks[2][0][2].getBlockKind(), stepInput, stepGlobalState);
        blockStepPartOutput +=
            stepFromPXCYNZ(stepInput.blocks[2][1][0].getBlockKind(), stepInput, stepGlobalState);
        blockStepPartOutput +=
            stepFromPXCYCZ(stepInput.blocks[2][1][1].getBlockKind(), stepInput, stepGlobalState);
        blockStepPartOutput +=
            stepFromPXCYPZ(stepInput.blocks[2][1][2].getBlockKind(), stepInput, stepGlobalState);
        blockStepPartOutput +=
            stepFromPXPYNZ(stepInput.blocks[2][2][0].getBlockKind(), stepInput, stepGlobalState);
        blockStepPartOutput +=
            stepFromPXPYCZ(stepInput.blocks[2][2][1].getBlockKind(), stepInput, stepGlobalState);
        blockStepPartOutput +=
            stepFromPXPYPZ(stepInput.blocks[2][2][2].getBlockKind(), stepInput, stepGlobalState);
        BlockKind outputBlockKind = blockStepPartOutput.blockKind == BlockKind::empty() ?
                                        stepInput.blocks[1][1][1].getBlockKind() :
                                        blockStepPartOutput.blockKind;
        return BlockStepFullOutput(
            Block(outputBlockKind,
                  get(outputBlockKind)
                      ->lightProperties.eval(
                          stepInput.blocks[0][1][1].getLightingIfNotEmpty(),
                          stepInput.blocks[2][1][1].getLightingIfNotEmpty(),
                          stepInput.blocks[1][0][1].getLightingIfNotEmpty(),
                          stepInput.blocks[1][2][1]
                              .getLighting(), // light from empty block only if above
                          stepInput.blocks[1][1][0].getLightingIfNotEmpty(),
                          stepInput.blocks[1][1][2].getLightingIfNotEmpty())),
            std::move(blockStepPartOutput.actions));
    }
    static bool needRenderBlockFace(const BlockDescriptor *neighborBlockDescriptor,
                                    BlockFace blockFace) noexcept
    {
        if(!neighborBlockDescriptor)
            return false;
        return !neighborBlockDescriptor->blockedFaces[reverse(blockFace)];
    }
    static bool needRenderBlockFace(BlockKind neighborBlockKind, BlockFace blockFace) noexcept
    {
        if(neighborBlockKind == BlockKind::empty())
            return false;
        return !get(neighborBlockKind)->blockedFaces[reverse(blockFace)];
    }
    static lighting::LightProperties getLightProperties(BlockKind blockKind) noexcept
    {
        if(!blockKind)
            return lighting::LightProperties::transparent();
        return get(blockKind)->lightProperties;
    }
    static BlockSummary getBlockSummary(BlockKind blockKind) noexcept
    {
        if(!blockKind)
            return BlockSummary::makeForEmptyBlockKind();
        return get(blockKind)->blockSummary;
    }
    static lighting::BlockLighting makeBlockLighting(const BlockStepInput &stepInput,
                                                     const BlockStepGlobalState &stepGlobalState,
                                                     util::Vector3I32 offset) noexcept
    {
        util::
            Array<util::Array<util::Array<std::pair<lighting::LightProperties, lighting::Lighting>,
                                          3>,
                              3>,
                  3> blocks;
        for(std::size_t x = 0; x < blocks.size(); x++)
        {
            for(std::size_t y = 0; y < blocks[0].size(); y++)
            {
                for(std::size_t z = 0; z < blocks[0][0].size(); z++)
                {
                    std::size_t blockX = x + offset.x;
                    std::size_t blockY = y + offset.y;
                    std::size_t blockZ = z + offset.z;
                    Block inputBlock = blockX < stepInput.blocks.size()
                                               && blockY < stepInput.blocks[0].size()
                                               && blockZ < stepInput.blocks[0][0].size() ?
                                           stepInput.blocks[x][y][z] :
                                           Block();
                    blocks[x][y][z] = {getLightProperties(inputBlock.getBlockKind()),
                                       inputBlock.getLighting()};
                }
            }
        }
        return lighting::BlockLighting(blocks, stepGlobalState.lightingGlobalProperties);
    }
};
}
}
}

#endif /* BLOCK_BLOCK_DESCRIPTOR_H_ */
