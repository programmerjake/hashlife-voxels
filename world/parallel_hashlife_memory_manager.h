/*
 * Copyright (C) 2012-2017 Jacob R. Lifshay
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

#ifndef WORLD_PARALLEL_HASHLIFE_MEMORY_MANAGER_H_
#define WORLD_PARALLEL_HASHLIFE_MEMORY_MANAGER_H_

#include "../util/integer_sequence.h"
#include <tuple>
#include <limits>

namespace programmerjake
{
namespace voxels
{
namespace world
{
namespace parallel_hashlife
{
template <std::size_t Level, typename DerivedType>
struct NodeBase;
class MemoryManager;

constexpr std::size_t MaxLevel = 32 - 2;

struct MemoryManagerNodeBase
{
    template <std::size_t Level, typename DerivedType>
    friend struct NodeBase;
    friend class MemoryManager;

private:
    static constexpr unsigned getNeededBitCount(std::size_t maxValue) noexcept
    {
        return maxValue == 0 ? 0 : 1 + getNeededBitCount(maxValue >> 1);
    }
    static constexpr unsigned levelBits = getNeededBitCount(MaxLevel);
    std::size_t level : levelBits;
    bool gcMark : 1;

private:
    template <typename Fn, std::size_t... Levels>
    static void dispatchHelper(const MemoryManagerNodeBase *node,
                               Fn &fn,
                               util::IntegerSequence<std::size_t, Levels...>);
    template <typename Fn>
    static void dispatch(const MemoryManagerNodeBase *node, Fn &&fn)
    {
        dispatchHelper<Fn>(node, fn, util::MakeIndexSequence<MaxLevel + 1>());
    }

private:
    template <std::size_t Level>
    constexpr MemoryManagerNodeBase(Node<Level> *) noexcept;
};
}
}
}
}

#include "parallel_hashlife_node.h"

namespace programmerjake
{
namespace voxels
{
namespace world
{
namespace parallel_hashlife
{
static_assert(Node<MaxLevel>::size != 0, "MaxLevel too big");

template <typename Fn, std::size_t... Levels>
void MemoryManagerNodeBase::dispatchHelper(const MemoryManagerNodeBase *node,
                                           Fn &fn,
                                           util::IntegerSequence<std::size_t, Levels...>)
{
    typedef void (*DispatchFnType)(const MemoryManagerNodeBase *node, Fn &fn);
    static const DispatchFnType functions[] = {
        static_cast<DispatchFnType>([](const MemoryManagerNodeBase *node, Fn &fn)
                                    {
                                        fn(static_cast<const Node<Levels> *>(node));
                                    })...};
    return functions[node->level](node, fn);
}

template <std::size_t Level>
constexpr MemoryManagerNodeBase::MemoryManagerNodeBase(Node<Level> *) noexcept : level(Level),
                                                                                 gcMark(false)
{
    static_assert(Level <= MaxLevel, "level too big");
}

class MemoryManager final
{
private:
    struct NodeBase
    {
#error finish
    };

public:
    template <std::size_t... ArgLevels>
    void collectGarbage(const std::tuple<Node<ArgLevels> *...> &roots) noexcept
    {
#error finish
    }
};
}
}
}
}

#endif /* WORLD_PARALLEL_HASHLIFE_MEMORY_MANAGER_H_ */
