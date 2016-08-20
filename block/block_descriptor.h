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
        return std::hash<const std::atomic_ptrdiff_t *>()(priority);
    }
};
}
}
}

namespace std
{
template <>
struct std::hash<programmerjake::voxels::block::BlockStepPriority>
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
    typedef std::function<void(world::World &world, world::Position3I32 position)> ActionFunction;
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
};

struct BlockStepPartOutput final
{
    BlockKind blockKind;
    BlockStepPriority priority;
    util::Optional<std::list<BlockStepExtraAction>> actions;
    constexpr bool empty() const
    {
        return blockKind == BlockKind::empty() && !actions.hasValue();
    }
    constexpr BlockStepPartOutput() : blockKind(BlockKind::empty()), priority(), actions()
    {
    }
    BlockStepPartOutput(BlockKind blockKind,
                        BlockStepPriority priority,
                        util::Optional<std::list<BlockStepExtraAction>> actions)
        : blockKind(blockKind), priority(priority), actions(std::move(actions))
    {
    }
    BlockStepPartOutput &addOffsetToExtraActions(util::Vector3I32 offset) &
    {
        if(actions.hasValue())
        {
            for(auto &action : *actions)
                action.addOffset(offset);
        }
        return *this;
    }
    BlockStepPartOutput &&addOffsetToExtraActions(util::Vector3I32 offset) &&
    {
        return std::move(addOffsetToExtraActions(offset));
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
        if(rt.actions.hasValue())
        {
            if(actions.hasValue())
                actions->splice(actions->end(), std::move(*rt.actions));
            else
                actions = std::move(rt.actions);
        }
        if(priority < rt.priority)
        {
            priority = rt.priority;
            blockKind = rt.blockKind;
        }
        return *this;
    }
    BlockStepPartOutput operator+(BlockStepPartOutput rt) const
    {
        auto retval = *this;
        retval += std::move(rt);
        return retval;
    }
};

struct BlockStepFullOutput final
{
    Block block;
    util::Optional<std::list<BlockStepExtraAction>> extraActions;
    constexpr BlockStepFullOutput() : block(), extraActions()
    {
    }
    BlockStepFullOutput(Block block, util::Optional<std::list<BlockStepExtraAction>> extraActions)
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

public:
    virtual ~BlockDescriptor() = default;
    const BlockKind blockKind;
    const lighting::LightProperties lightProperties;
    BlockDescriptor() noexcept;
    virtual BlockStepPartOutput stepNXNYNZ(const BlockStepInput &stepInput) const
    {
        return BlockStepPartOutput();
    }
    virtual BlockStepPartOutput stepNXNYCZ(const BlockStepInput &stepInput) const
    {
        return BlockStepPartOutput();
    }
    virtual BlockStepPartOutput stepNXNYPZ(const BlockStepInput &stepInput) const
    {
        return BlockStepPartOutput();
    }
    virtual BlockStepPartOutput stepNXCYNZ(const BlockStepInput &stepInput) const
    {
        return BlockStepPartOutput();
    }
    virtual BlockStepPartOutput stepNXCYCZ(const BlockStepInput &stepInput) const
    {
        return BlockStepPartOutput();
    }
    virtual BlockStepPartOutput stepNXCYPZ(const BlockStepInput &stepInput) const
    {
        return BlockStepPartOutput();
    }
    virtual BlockStepPartOutput stepNXPYNZ(const BlockStepInput &stepInput) const
    {
        return BlockStepPartOutput();
    }
    virtual BlockStepPartOutput stepNXPYCZ(const BlockStepInput &stepInput) const
    {
        return BlockStepPartOutput();
    }
    virtual BlockStepPartOutput stepNXPYPZ(const BlockStepInput &stepInput) const
    {
        return BlockStepPartOutput();
    }
    virtual BlockStepPartOutput stepCXNYNZ(const BlockStepInput &stepInput) const
    {
        return BlockStepPartOutput();
    }
    virtual BlockStepPartOutput stepCXNYCZ(const BlockStepInput &stepInput) const
    {
        return BlockStepPartOutput();
    }
    virtual BlockStepPartOutput stepCXNYPZ(const BlockStepInput &stepInput) const
    {
        return BlockStepPartOutput();
    }
    virtual BlockStepPartOutput stepCXCYNZ(const BlockStepInput &stepInput) const
    {
        return BlockStepPartOutput();
    }
    virtual BlockStepPartOutput stepCXCYCZ(const BlockStepInput &stepInput) const
    {
        return BlockStepPartOutput();
    }
    virtual BlockStepPartOutput stepCXCYPZ(const BlockStepInput &stepInput) const
    {
        return BlockStepPartOutput();
    }
    virtual BlockStepPartOutput stepCXPYNZ(const BlockStepInput &stepInput) const
    {
        return BlockStepPartOutput();
    }
    virtual BlockStepPartOutput stepCXPYCZ(const BlockStepInput &stepInput) const
    {
        return BlockStepPartOutput();
    }
    virtual BlockStepPartOutput stepCXPYPZ(const BlockStepInput &stepInput) const
    {
        return BlockStepPartOutput();
    }
    virtual BlockStepPartOutput stepPXNYNZ(const BlockStepInput &stepInput) const
    {
        return BlockStepPartOutput();
    }
    virtual BlockStepPartOutput stepPXNYCZ(const BlockStepInput &stepInput) const
    {
        return BlockStepPartOutput();
    }
    virtual BlockStepPartOutput stepPXNYPZ(const BlockStepInput &stepInput) const
    {
        return BlockStepPartOutput();
    }
    virtual BlockStepPartOutput stepPXCYNZ(const BlockStepInput &stepInput) const
    {
        return BlockStepPartOutput();
    }
    virtual BlockStepPartOutput stepPXCYCZ(const BlockStepInput &stepInput) const
    {
        return BlockStepPartOutput();
    }
    virtual BlockStepPartOutput stepPXCYPZ(const BlockStepInput &stepInput) const
    {
        return BlockStepPartOutput();
    }
    virtual BlockStepPartOutput stepPXPYNZ(const BlockStepInput &stepInput) const
    {
        return BlockStepPartOutput();
    }
    virtual BlockStepPartOutput stepPXPYCZ(const BlockStepInput &stepInput) const
    {
        return BlockStepPartOutput();
    }
    virtual BlockStepPartOutput stepPXPYPZ(const BlockStepInput &stepInput) const
    {
        return BlockStepPartOutput();
    }
    static BlockStepPartOutput stepNXNYNZ(BlockKind blockKind, const BlockStepInput &stepInput)
    {
        if(blockKind == BlockKind::empty())
            return BlockStepPartOutput();
        return get(blockKind)->stepNXNYNZ(stepInput);
    }
    static BlockStepPartOutput stepNXNYCZ(BlockKind blockKind, const BlockStepInput &stepInput)
    {
        if(blockKind == BlockKind::empty())
            return BlockStepPartOutput();
        return get(blockKind)->stepNXNYCZ(stepInput);
    }
    static BlockStepPartOutput stepNXNYPZ(BlockKind blockKind, const BlockStepInput &stepInput)
    {
        if(blockKind == BlockKind::empty())
            return BlockStepPartOutput();
        return get(blockKind)->stepNXNYPZ(stepInput);
    }
    static BlockStepPartOutput stepNXCYNZ(BlockKind blockKind, const BlockStepInput &stepInput)
    {
        if(blockKind == BlockKind::empty())
            return BlockStepPartOutput();
        return get(blockKind)->stepNXCYNZ(stepInput);
    }
    static BlockStepPartOutput stepNXCYCZ(BlockKind blockKind, const BlockStepInput &stepInput)
    {
        if(blockKind == BlockKind::empty())
            return BlockStepPartOutput();
        return get(blockKind)->stepNXCYCZ(stepInput);
    }
    static BlockStepPartOutput stepNXCYPZ(BlockKind blockKind, const BlockStepInput &stepInput)
    {
        if(blockKind == BlockKind::empty())
            return BlockStepPartOutput();
        return get(blockKind)->stepNXCYPZ(stepInput);
    }
    static BlockStepPartOutput stepNXPYNZ(BlockKind blockKind, const BlockStepInput &stepInput)
    {
        if(blockKind == BlockKind::empty())
            return BlockStepPartOutput();
        return get(blockKind)->stepNXPYNZ(stepInput);
    }
    static BlockStepPartOutput stepNXPYCZ(BlockKind blockKind, const BlockStepInput &stepInput)
    {
        if(blockKind == BlockKind::empty())
            return BlockStepPartOutput();
        return get(blockKind)->stepNXPYCZ(stepInput);
    }
    static BlockStepPartOutput stepNXPYPZ(BlockKind blockKind, const BlockStepInput &stepInput)
    {
        if(blockKind == BlockKind::empty())
            return BlockStepPartOutput();
        return get(blockKind)->stepNXPYPZ(stepInput);
    }
    static BlockStepPartOutput stepCXNYNZ(BlockKind blockKind, const BlockStepInput &stepInput)
    {
        if(blockKind == BlockKind::empty())
            return BlockStepPartOutput();
        return get(blockKind)->stepCXNYNZ(stepInput);
    }
    static BlockStepPartOutput stepCXNYCZ(BlockKind blockKind, const BlockStepInput &stepInput)
    {
        if(blockKind == BlockKind::empty())
            return BlockStepPartOutput();
        return get(blockKind)->stepCXNYCZ(stepInput);
    }
    static BlockStepPartOutput stepCXNYPZ(BlockKind blockKind, const BlockStepInput &stepInput)
    {
        if(blockKind == BlockKind::empty())
            return BlockStepPartOutput();
        return get(blockKind)->stepCXNYPZ(stepInput);
    }
    static BlockStepPartOutput stepCXCYNZ(BlockKind blockKind, const BlockStepInput &stepInput)
    {
        if(blockKind == BlockKind::empty())
            return BlockStepPartOutput();
        return get(blockKind)->stepCXCYNZ(stepInput);
    }
    static BlockStepPartOutput stepCXCYCZ(BlockKind blockKind, const BlockStepInput &stepInput)
    {
        if(blockKind == BlockKind::empty())
            return BlockStepPartOutput();
        return get(blockKind)->stepCXCYCZ(stepInput);
    }
    static BlockStepPartOutput stepCXCYPZ(BlockKind blockKind, const BlockStepInput &stepInput)
    {
        if(blockKind == BlockKind::empty())
            return BlockStepPartOutput();
        return get(blockKind)->stepCXCYPZ(stepInput);
    }
    static BlockStepPartOutput stepCXPYNZ(BlockKind blockKind, const BlockStepInput &stepInput)
    {
        if(blockKind == BlockKind::empty())
            return BlockStepPartOutput();
        return get(blockKind)->stepCXPYNZ(stepInput);
    }
    static BlockStepPartOutput stepCXPYCZ(BlockKind blockKind, const BlockStepInput &stepInput)
    {
        if(blockKind == BlockKind::empty())
            return BlockStepPartOutput();
        return get(blockKind)->stepCXPYCZ(stepInput);
    }
    static BlockStepPartOutput stepCXPYPZ(BlockKind blockKind, const BlockStepInput &stepInput)
    {
        if(blockKind == BlockKind::empty())
            return BlockStepPartOutput();
        return get(blockKind)->stepCXPYPZ(stepInput);
    }
    static BlockStepPartOutput stepPXNYNZ(BlockKind blockKind, const BlockStepInput &stepInput)
    {
        if(blockKind == BlockKind::empty())
            return BlockStepPartOutput();
        return get(blockKind)->stepPXNYNZ(stepInput);
    }
    static BlockStepPartOutput stepPXNYCZ(BlockKind blockKind, const BlockStepInput &stepInput)
    {
        if(blockKind == BlockKind::empty())
            return BlockStepPartOutput();
        return get(blockKind)->stepPXNYCZ(stepInput);
    }
    static BlockStepPartOutput stepPXNYPZ(BlockKind blockKind, const BlockStepInput &stepInput)
    {
        if(blockKind == BlockKind::empty())
            return BlockStepPartOutput();
        return get(blockKind)->stepPXNYPZ(stepInput);
    }
    static BlockStepPartOutput stepPXCYNZ(BlockKind blockKind, const BlockStepInput &stepInput)
    {
        if(blockKind == BlockKind::empty())
            return BlockStepPartOutput();
        return get(blockKind)->stepPXCYNZ(stepInput);
    }
    static BlockStepPartOutput stepPXCYCZ(BlockKind blockKind, const BlockStepInput &stepInput)
    {
        if(blockKind == BlockKind::empty())
            return BlockStepPartOutput();
        return get(blockKind)->stepPXCYCZ(stepInput);
    }
    static BlockStepPartOutput stepPXCYPZ(BlockKind blockKind, const BlockStepInput &stepInput)
    {
        if(blockKind == BlockKind::empty())
            return BlockStepPartOutput();
        return get(blockKind)->stepPXCYPZ(stepInput);
    }
    static BlockStepPartOutput stepPXPYNZ(BlockKind blockKind, const BlockStepInput &stepInput)
    {
        if(blockKind == BlockKind::empty())
            return BlockStepPartOutput();
        return get(blockKind)->stepPXPYNZ(stepInput);
    }
    static BlockStepPartOutput stepPXPYCZ(BlockKind blockKind, const BlockStepInput &stepInput)
    {
        if(blockKind == BlockKind::empty())
            return BlockStepPartOutput();
        return get(blockKind)->stepPXPYCZ(stepInput);
    }
    static BlockStepPartOutput stepPXPYPZ(BlockKind blockKind, const BlockStepInput &stepInput)
    {
        if(blockKind == BlockKind::empty())
            return BlockStepPartOutput();
        return get(blockKind)->stepPXPYPZ(stepInput);
    }
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
    static BlockStepFullOutput step(const BlockStepInput &stepInput)
    {
        if(stepInput.blocks[1][1][1].getBlockKind() == BlockKind::empty())
            return stepInput.blocks[1][1][1];
        BlockStepPartOutput blockStepPartOutput =
            stepNXNYNZ(stepInput.blocks[0][0][0].getBlockKind(), stepInput);
        blockStepPartOutput += stepNXNYCZ(stepInput.blocks[0][0][1].getBlockKind(), stepInput);
        blockStepPartOutput += stepNXNYPZ(stepInput.blocks[0][0][2].getBlockKind(), stepInput);
        blockStepPartOutput += stepNXCYNZ(stepInput.blocks[0][1][0].getBlockKind(), stepInput);
        blockStepPartOutput += stepNXCYCZ(stepInput.blocks[0][1][1].getBlockKind(), stepInput);
        blockStepPartOutput += stepNXCYPZ(stepInput.blocks[0][1][2].getBlockKind(), stepInput);
        blockStepPartOutput += stepNXPYNZ(stepInput.blocks[0][2][0].getBlockKind(), stepInput);
        blockStepPartOutput += stepNXPYCZ(stepInput.blocks[0][2][1].getBlockKind(), stepInput);
        blockStepPartOutput += stepNXPYPZ(stepInput.blocks[0][2][2].getBlockKind(), stepInput);
        blockStepPartOutput += stepCXNYNZ(stepInput.blocks[1][0][0].getBlockKind(), stepInput);
        blockStepPartOutput += stepCXNYCZ(stepInput.blocks[1][0][1].getBlockKind(), stepInput);
        blockStepPartOutput += stepCXNYPZ(stepInput.blocks[1][0][2].getBlockKind(), stepInput);
        blockStepPartOutput += stepCXCYNZ(stepInput.blocks[1][1][0].getBlockKind(), stepInput);
        blockStepPartOutput += stepCXCYCZ(stepInput.blocks[1][1][1].getBlockKind(), stepInput);
        blockStepPartOutput += stepCXCYPZ(stepInput.blocks[1][1][2].getBlockKind(), stepInput);
        blockStepPartOutput += stepCXPYNZ(stepInput.blocks[1][2][0].getBlockKind(), stepInput);
        blockStepPartOutput += stepCXPYCZ(stepInput.blocks[1][2][1].getBlockKind(), stepInput);
        blockStepPartOutput += stepCXPYPZ(stepInput.blocks[1][2][2].getBlockKind(), stepInput);
        blockStepPartOutput += stepPXNYNZ(stepInput.blocks[2][0][0].getBlockKind(), stepInput);
        blockStepPartOutput += stepPXNYCZ(stepInput.blocks[2][0][1].getBlockKind(), stepInput);
        blockStepPartOutput += stepPXNYPZ(stepInput.blocks[2][0][2].getBlockKind(), stepInput);
        blockStepPartOutput += stepPXCYNZ(stepInput.blocks[2][1][0].getBlockKind(), stepInput);
        blockStepPartOutput += stepPXCYCZ(stepInput.blocks[2][1][1].getBlockKind(), stepInput);
        blockStepPartOutput += stepPXCYPZ(stepInput.blocks[2][1][2].getBlockKind(), stepInput);
        blockStepPartOutput += stepPXPYNZ(stepInput.blocks[2][2][0].getBlockKind(), stepInput);
        blockStepPartOutput += stepPXPYCZ(stepInput.blocks[2][2][1].getBlockKind(), stepInput);
        blockStepPartOutput += stepPXPYPZ(stepInput.blocks[2][2][2].getBlockKind(), stepInput);
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
