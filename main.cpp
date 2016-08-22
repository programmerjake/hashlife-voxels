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
#include "world/world.h"
#include "world/init.h"
#include "block/block_descriptor.h"
#include "util/enum.h"
#include "logging/logging.h"
#include "util/constexpr_assert.h"
#include "block/builtin/air.h"
#include <sstream>

namespace programmerjake
{
namespace voxels
{
namespace
{
struct MyBlock final : public block::BlockDescriptor
{
    enum class State
    {
        Started,
        Done,
        DEFINE_ENUM_MAX(Done)
    };

private:
    explicit MyBlock(State state)
        : BlockDescriptor("testing.myBlock", lighting::LightProperties::opaque()), state(state)
    {
    }
    static std::array<const MyBlock *, util::EnumTraits<State>::size> make()
    {
        std::array<const MyBlock *, util::EnumTraits<State>::size> retval;
        for(auto state : util::EnumTraits<State>::values)
        {
            retval[util::EnumIterator<State>(state) - util::EnumTraits<State>::values.begin()] =
                new MyBlock(state);
        }
        return retval;
    }

public:
    const State state;
    static const std::array<const MyBlock *, util::EnumTraits<State>::size> &get()
    {
        static std::array<const MyBlock *, util::EnumTraits<State>::size> retval = make();
        return retval;
    }
    static const MyBlock *get(State state)
    {
        return get()[util::EnumIterator<State>(state) - util::EnumTraits<State>::values.begin()];
    }
    static void init()
    {
        get();
    }
    virtual block::BlockStepPartOutput stepCXCYCZ(
        const block::BlockStepInput &stepInput,
        const block::BlockStepGlobalState &stepGlobalState) const override
    {
        switch(state)
        {
        case State::Done:
            return {};
        case State::Started:
        {
            return block::BlockStepPartOutput(
                get(State::Done)->blockKind,
                block::BlockStepExtraAction([](world::World &world, world::Position3I32 position)
                                            {
                                                std::ostringstream ss;
                                                ss << position.x << " " << position.y << " "
                                                   << position.z << " " << position.d.getName();
                                                logging::log(
                                                    logging::Level::Info, "MyBlock", ss.str());
                                            }));
        }
        }
        constexprAssert(false);
        return {};
    }
    virtual block::BlockStepPartOutput stepNXCYCZ(
        const block::BlockStepInput &stepInput,
        const block::BlockStepGlobalState &stepGlobalState) const override
    {
        if(dynamic_cast<const MyBlock *>(
               BlockDescriptor::get(stepInput.blocks[1][1][1].getBlockKind())))
            return {};
        return {get(State::Started)->blockKind};
    }
    virtual block::BlockStepPartOutput stepPXCYCZ(
        const block::BlockStepInput &stepInput,
        const block::BlockStepGlobalState &stepGlobalState) const override
    {
        if(dynamic_cast<const MyBlock *>(
               BlockDescriptor::get(stepInput.blocks[1][1][1].getBlockKind())))
            return {};
        return {get(State::Started)->blockKind};
    }
    virtual block::BlockStepPartOutput stepCXNYCZ(
        const block::BlockStepInput &stepInput,
        const block::BlockStepGlobalState &stepGlobalState) const override
    {
        if(dynamic_cast<const MyBlock *>(
               BlockDescriptor::get(stepInput.blocks[1][1][1].getBlockKind())))
            return {};
        return {get(State::Started)->blockKind};
    }
    virtual block::BlockStepPartOutput stepCXPYCZ(
        const block::BlockStepInput &stepInput,
        const block::BlockStepGlobalState &stepGlobalState) const override
    {
        if(dynamic_cast<const MyBlock *>(
               BlockDescriptor::get(stepInput.blocks[1][1][1].getBlockKind())))
            return {};
        return {get(State::Started)->blockKind};
    }
    virtual block::BlockStepPartOutput stepCXCYNZ(
        const block::BlockStepInput &stepInput,
        const block::BlockStepGlobalState &stepGlobalState) const override
    {
        if(dynamic_cast<const MyBlock *>(
               BlockDescriptor::get(stepInput.blocks[1][1][1].getBlockKind())))
            return {};
        return {get(State::Started)->blockKind};
    }
    virtual block::BlockStepPartOutput stepCXCYPZ(
        const block::BlockStepInput &stepInput,
        const block::BlockStepGlobalState &stepGlobalState) const override
    {
        if(dynamic_cast<const MyBlock *>(
               BlockDescriptor::get(stepInput.blocks[1][1][1].getBlockKind())))
            return {};
        return {get(State::Started)->blockKind};
    }
};
int main()
{
    world::initAll();
    MyBlock::init();
    world::World theWorld;
    std::array<std::array<std::array<block::Block, 10>, 10>, 10> blocks;
    for(auto &i : blocks)
        for(auto &j : i)
            for(auto &block : j)
                block = block::Block(block::builtin::Air::get()->blockKind);
    theWorld.hashlifeWorld->setBlocks(
        blocks, util::Vector3I32(-5), util::Vector3I32(0), util::Vector3I32(10));
    for(std::size_t step = 0; step < 20; step++)
    {
        auto actions = theWorld.hashlifeWorld->stepAndCollectGarbage(
            block::BlockStepGlobalState(lighting::Lighting::GlobalProperties(
                lighting::Lighting::maxLight, world::Dimension::overworld())));
        actions.run(theWorld, world::Dimension::overworld());
    }
    return 0;
}
}
}
}

int main()
{
    return programmerjake::voxels::main();
}
