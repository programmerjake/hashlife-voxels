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
#include <memory>
#include <functional>
#include <type_traits>
#include <mutex>
#include <cstdint>
#include <new>
#include <cassert>
#include "../util/bitset.h"

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
    bool gcMark : 1;
    constexpr MemoryManagerNodeBase() noexcept : gcMark(false)
    {
    }
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
class MemoryManager final
{
private:
    struct NodeGroupBase
    {
        NodeGroupBase(const NodeGroupBase &) = delete;
        NodeGroupBase &operator=(const NodeGroupBase &) = delete;
        virtual ~NodeGroupBase() = default;
        const MemoryManagerNodeBase *const startAddress;
        const MemoryManagerNodeBase *const endAddress;
        const std::size_t level;
        std::size_t nextAllocateIndex;
        std::size_t usedCount;
        constexpr NodeGroupBase(const MemoryManagerNodeBase *const startAddress,
                                const MemoryManagerNodeBase *const endAddress,
                                std::size_t level) noexcept : startAddress(startAddress),
                                                              endAddress(endAddress),
                                                              level(level),
                                                              nextAllocateIndex(0),
                                                              usedCount(0)
        {
        }
    };
    struct NodeGroupBasePointerLess
    {
        template <
            typename T1,
            typename T2,
            typename = typename std::enable_if<std::is_base_of<NodeGroupBase, T1>::value
                                               && std::is_base_of<NodeGroupBase, T2>::value>::type>
        bool operator()(const std::unique_ptr<T1> &a, const std::unique_ptr<T2> &b) const noexcept
        {
            return std::less<const NodeGroupBase *>()(a.get(), b.get());
        }
        template <
            typename T,
            typename = typename std::enable_if<std::is_base_of<NodeGroupBase, T>::value>::type>
        bool operator()(const std::unique_ptr<T> &a, const NodeGroupBase *b) const noexcept
        {
            return std::less<const NodeGroupBase *>()(a.get(), b);
        }
        template <
            typename T,
            typename = typename std::enable_if<std::is_base_of<NodeGroupBase, T>::value>::type>
        bool operator()(const NodeGroupBase *a, const std::unique_ptr<T> &b) const noexcept
        {
            return std::less<const NodeGroupBase *>()(a, b.get());
        }
        bool operator()(const NodeGroupBase *a, const NodeGroupBase *b) const noexcept
        {
            return std::less<const NodeGroupBase *>()(a, b);
        }
    };
    static constexpr std::size_t nodeGroupSizeTarget = 1UL << 20; // 1MiB
    template <std::size_t Level>
    struct NodeGroup final : public NodeGroupBase
    {
        typedef Node<Level> NodeType;
        static constexpr std::size_t allocatedCount =
            (nodeGroupSizeTarget - sizeof(NodeGroupBase)) / sizeof(NodeType);
        static_assert(allocatedCount != 0, "nodeGroupSizeTarget is too small");
        typename std::aligned_storage<sizeof(Node<Level>), alignof(Node<Level>)>::type
            nodes[allocatedCount];
        util::BitSet<allocatedCount> usedSet;
        std::size_t lastAllocateIndex;
        NodeGroup() noexcept : NodeGroupBase(nodes, nodes + allocatedCount, Level),
                               nodes(),
                               usedSet(),
                               lastAllocateIndex(0)
        {
            usedSet.set();
        }
        Node<Level> *allocate(const typename Node<Level>::Key &key,
                              const block::BlockSummary &blockSummary) noexcept
        {
            std::size_t index = usedSet.findFirst(false, lastAllocateIndex);
            if(index >= allocatedCount)
            {
                index = usedSet.findLast(false, lastAllocateIndex);
                if(index >= allocatedCount)
                {
                    lastAllocateIndex = 0;
                    return nullptr;
                }
            }
            lastAllocateIndex = index;
            return ::new(&nodes[index]) Node<Level>(key, blockSummary);
        }
        void free(Node<Level> *node) noexcept
        {
            std::size_t index = node - nodes;
            assert(index < allocatedCount && node == nodes + index);
            assert(usedSet[index]);
            reinterpret_cast<Node<Level> &>(nodes[index]).~Node<Level>();
            usedSet[index] = false;
        }
        virtual ~NodeGroup() override
        {
            for(std::size_t i = allocatedCount; i > 0; i--)
            {
                if(usedSet[i - 1])
                    reinterpret_cast<Node<Level> &>(nodes[i - 1]).~Node<Level>();
            }
        }
    };
    std::vector<std::unique_ptr<>>

        public : template <std::size_t... ArgLevels>
                 void
                 collectGarbage(const std::tuple<Node<ArgLevels> *...> &roots) noexcept
    {
#error finish
    }
};
}
}
}
}

#endif /* WORLD_PARALLEL_HASHLIFE_MEMORY_MANAGER_H_ */
