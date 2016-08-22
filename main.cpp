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
#include <iostream>

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
    static const char *getStateName(State state) noexcept
    {
        switch(state)
        {
        case State::Started:
            return "Started";
        case State::Done:
            return "Done";
        }
        constexprAssert(false);
        return "<unknown>";
    }
    static std::string getName(State state, std::uint32_t generation)
    {
        std::ostringstream ss;
        ss << "testing.myBlock(";
        ss << getStateName(state);
        ss << ",";
        ss << generation;
        ss << ")";
        return ss.str();
    }
    explicit MyBlock(State state, std::uint32_t generation)
        : BlockDescriptor(getName(state, generation), lighting::LightProperties::opaque()),
          state(state),
          generation(generation)
    {
    }
    static std::array<std::array<const MyBlock *,
                                 block::BlockStepGlobalState::stepSizeInGenerations>,
                      util::EnumTraits<State>::size>
        make()
    {
        std::array<std::array<const MyBlock *, block::BlockStepGlobalState::stepSizeInGenerations>,
                   util::EnumTraits<State>::size> retval;
        for(auto state : util::EnumTraits<State>::values)
        {
            for(std::uint32_t generation = 0;
                generation < block::BlockStepGlobalState::stepSizeInGenerations;
                generation++)
            {
                retval[util::EnumIterator<State>(state)
                       - util::EnumTraits<State>::values.begin()][generation] =
                    new MyBlock(state, generation);
            }
        }
        return retval;
    }

public:
    const State state;
    const std::uint32_t generation;
    static const std::array<std::array<const MyBlock *,
                                       block::BlockStepGlobalState::stepSizeInGenerations>,
                            util::EnumTraits<State>::size> &
        get()
    {
        static const std::array<std::array<const MyBlock *,
                                           block::BlockStepGlobalState::stepSizeInGenerations>,
                                util::EnumTraits<State>::size> retval = make();
        return retval;
    }
    static const MyBlock *get(State state, std::uint32_t generation)
    {
        return get()[util::EnumIterator<State>(state)
                     - util::EnumTraits<State>::values.begin()][generation];
    }
    static void init()
    {
        get();
    }
    virtual block::BlockStepPartOutput stepFromCXCYCZ(
        const block::BlockStepInput &stepInput,
        const block::BlockStepGlobalState &stepGlobalState) const override
    {
#if 1
        switch(state)
        {
        case State::Done:
            if(generation == 0)
                return {block::builtin::Air::get()->blockKind};
            return {get(State::Done,
                        (generation + 1) % block::BlockStepGlobalState::stepSizeInGenerations)->blockKind};
        case State::Started:
        {
            return block::BlockStepPartOutput(
                get(State::Done,
                    (generation + 1) % block::BlockStepGlobalState::stepSizeInGenerations)
                    ->blockKind,
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
#endif
        return {};
    }
    virtual block::BlockStepPartOutput stepFromCXNYCZ(
        const block::BlockStepInput &stepInput,
        const block::BlockStepGlobalState &stepGlobalState) const override
    {
        if(dynamic_cast<const MyBlock *>(
               BlockDescriptor::get(stepInput.blocks[1][1][1].getBlockKind())))
            return {};
        return {
            get(State::Started,
                (generation + 1) % block::BlockStepGlobalState::stepSizeInGenerations)->blockKind};
    }
#if 1
    virtual block::BlockStepPartOutput stepFromNXCYCZ(
        const block::BlockStepInput &stepInput,
        const block::BlockStepGlobalState &stepGlobalState) const override
    {
        if(dynamic_cast<const MyBlock *>(
               BlockDescriptor::get(stepInput.blocks[1][1][1].getBlockKind())))
            return {};
        return {
            get(State::Started,
                (generation + 1) % block::BlockStepGlobalState::stepSizeInGenerations)->blockKind};
    }
    virtual block::BlockStepPartOutput stepFromCXCYNZ(
        const block::BlockStepInput &stepInput,
        const block::BlockStepGlobalState &stepGlobalState) const override
    {
        if(dynamic_cast<const MyBlock *>(
               BlockDescriptor::get(stepInput.blocks[1][1][1].getBlockKind())))
            return {};
        return {
            get(State::Started,
                (generation + 1) % block::BlockStepGlobalState::stepSizeInGenerations)->blockKind};
    }
    virtual block::BlockStepPartOutput stepFromPXCYCZ(
        const block::BlockStepInput &stepInput,
        const block::BlockStepGlobalState &stepGlobalState) const override
    {
        if(dynamic_cast<const MyBlock *>(
               BlockDescriptor::get(stepInput.blocks[1][1][1].getBlockKind())))
            return {};
        return {
            get(State::Started,
                (generation + 1) % block::BlockStepGlobalState::stepSizeInGenerations)->blockKind};
    }
    virtual block::BlockStepPartOutput stepFromCXPYCZ(
        const block::BlockStepInput &stepInput,
        const block::BlockStepGlobalState &stepGlobalState) const override
    {
        if(dynamic_cast<const MyBlock *>(
               BlockDescriptor::get(stepInput.blocks[1][1][1].getBlockKind())))
            return {};
        return {
            get(State::Started,
                (generation + 1) % block::BlockStepGlobalState::stepSizeInGenerations)->blockKind};
    }
    virtual block::BlockStepPartOutput stepFromCXCYPZ(
        const block::BlockStepInput &stepInput,
        const block::BlockStepGlobalState &stepGlobalState) const override
    {
        if(dynamic_cast<const MyBlock *>(
               BlockDescriptor::get(stepInput.blocks[1][1][1].getBlockKind())))
            return {};
        return {
            get(State::Started,
                (generation + 1) % block::BlockStepGlobalState::stepSizeInGenerations)->blockKind};
    }
#endif
};
template <typename>
struct DumpAccessArrayWrapper;
template <typename T>
struct IsArray
{
    static constexpr bool value = false;
    static T &addWrapper(T &value) noexcept
    {
        std::cout << " -> " << block::BlockDescriptor::get(value.getBlockKind())->name << std::endl;
        return value;
    }
};
template <typename T, std::size_t N>
struct IsArray<std::array<T, N>>
{
    static constexpr bool value = true;
    static DumpAccessArrayWrapper<std::array<T, N>> addWrapper(std::array<T, N> &value) noexcept
    {
        return DumpAccessArrayWrapper<std::array<T, N>>(value);
    }
};

template <typename T, std::size_t N>
struct DumpAccessArrayWrapper<std::array<T, N>>
{
    std::array<T, N> &value;
    DumpAccessArrayWrapper(std::array<T, N> &value) : value(value)
    {
    }
    std::size_t size() const noexcept
    {
        std::cout << ".size()" << std::endl;
        return N;
    }
    decltype(IsArray<T>::addWrapper(std::declval<T &>())) operator[](std::size_t index) noexcept
    {
        std::cout << "[" << index << "]";
        return IsArray<T>::addWrapper(value[index]);
    }
};

int main()
{
    world::initAll();
    MyBlock::init();
    logging::setGlobalLevel(logging::Level::Debug);
    world::World theWorld;
    constexpr std::size_t blocksSize = 8;
    typedef std::array<std::array<std::array<block::Block, blocksSize>, blocksSize>, blocksSize>
        Blocks;
    Blocks blocks;
    for(auto &i : blocks)
        for(auto &j : i)
            for(auto &block : j)
                block = block::Block(block::builtin::Air::get()->blockKind);
    theWorld.hashlifeWorld->setBlocks(blocks,
                                      -util::Vector3I32(blocksSize / 2),
                                      util::Vector3I32(0),
                                      util::Vector3I32(blocksSize));
    theWorld.hashlifeWorld->setBlock(
        block::Block(MyBlock::get(MyBlock::State::Started, 0)->blockKind), util::Vector3I32(0));
    auto snapshot = theWorld.hashlifeWorld->makeSnapshot();
    for(std::size_t step = 0; step < 2000; step++)
    {
        snapshot = theWorld.hashlifeWorld->makeSnapshot();
        auto actions = theWorld.hashlifeWorld->stepAndCollectGarbage(
            block::BlockStepGlobalState(lighting::Lighting::GlobalProperties(
                lighting::Lighting::maxLight, world::Dimension::overworld())));
        actions.run(theWorld, world::Dimension::overworld());
#if 1
        std::cout << step << std::endl;
        constexpr std::int32_t searchDistance = blocksSize / 2;
        for(util::Vector3I32 p(-searchDistance); p.x < searchDistance || p.x == 0; p.x++)
        {
            constexpr auto columnWidth = 5;
            std::cout.width(columnWidth);
            std::cout << p.x;
            std::cout.width(columnWidth);
            std::cout << "";
            for(std::int32_t i = -searchDistance; i < searchDistance || i == 0; i++)
            {
                std::cout.width(columnWidth);
                std::cout << i;
            }
            std::cout << std::endl;
            std::cout << std::endl;
            for(p.y = -searchDistance; p.y < searchDistance || p.y == 0; p.y++)
            {
                std::cout.width(columnWidth);
                std::cout << p.y;
                std::cout.width(columnWidth);
                std::cout << "";
                for(p.z = -searchDistance; p.z < searchDistance || p.z == 0; p.z++)
                {
                    auto blockKind = snapshot->get(p).getBlockKind();
                    std::cout.width(columnWidth);
                    std::cout << blockKind.value;
                }
                std::cout << std::endl;
            }
            std::cout << std::endl;
            std::cout << std::endl;
        }
#endif
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
