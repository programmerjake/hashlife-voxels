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
#include <deque>
#include <random>
#include "../util/bitset.h"
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
    struct NodeGroupListEntry final
    {
        std::shared_ptr<NodeGroupBase> nodeGroup;
        const MemoryManagerNodeBase *startAddress;
        const MemoryManagerNodeBase *endAddress;
        std::size_t level;
        explicit NodeGroupListEntry(std::shared_ptr<NodeGroupBase> nodeGroup) noexcept
            : nodeGroup(std::move(nodeGroup)),
              startAddress(this->nodeGroup->startAddress),
              endAddress(this->nodeGroup->endAddress),
              level(this->nodeGroup->level)
        {
        }
        bool contained(const MemoryManagerNodeBase *address) const noexcept
        {
            typedef std::less<const MemoryManagerNodeBase *> nodePtrLess;
            return !nodePtrLess()(address, startAddress) && nodePtrLess()(address, endAddress);
        }
    };
    struct NodeGroupListEntryLess
    {
        bool operator()(const NodeGroupListEntry &a, const NodeGroupListEntry &b) const noexcept
        {
            return std::less<const MemoryManagerNodeBase *>()(a.startAddress, b.startAddress);
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
            usedCount++;
            return ::new(&nodes[index]) Node<Level>(key, blockSummary);
        }
        void free(Node<Level> *node) noexcept
        {
            std::size_t index = node - reinterpret_cast<Node<Level> *>(nodes);
            assert(index < allocatedCount
                   && node == reinterpret_cast<Node<Level> *>(nodes) + index);
            assert(usedSet[index]);
            reinterpret_cast<Node<Level> &>(nodes[index]).~Node<Level>();
            usedSet[index] = false;
            usedCount--;
        }
        template <typename Fn>
        void forEachAllocated(Fn &&fn) noexcept(noexcept(fn(static_cast<Node<Level> *>(nullptr))))
        {
            // must work when fn modifies this NodeGroup
            if(usedCount == 0)
                return;
            for(std::size_t index = usedSet.findFirst(true); index != usedSet.npos;
                index = usedSet.findFirst(true, index + 1))
            {
                fn(reinterpret_cast<Node<Level> *>(&nodes[index]));
            }
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
    std::array<std::vector<NodeGroupListEntry>, LevelCount> nodeGroupLists;
    template <typename Runner, std::size_t... Levels>
    static void runForEachLevelHelper(
        Runner &runner,
        util::IntegerSequence<std::size_t,
                              Levels...>) noexcept(noexcept(std::initializer_list<char>{
        (static_cast<void>(runner.template run<Levels>()), '\0')...}))
    {
        std::initializer_list<char>{(static_cast<void>(runner.template run<Levels>()), '\0')...};
    }
    template <typename Runner>
    static void runForEachLevel(Runner &&runner) noexcept(
        noexcept(runForEachLevelHelper<Runner>(runner, util::MakeIndexSequence<LevelCount>())))
    {
        runForEachLevelHelper<Runner>(runner, util::MakeIndexSequence<LevelCount>());
    }
    template <typename Runner>
    struct RunForEachNode
    {
        Runner &runner;
        MemoryManager *memoryManager;
        constexpr RunForEachNode(Runner &runner, MemoryManager *memoryManager) noexcept
            : runner(runner),
              memoryManager(memoryManager)
        {
        }
        template <std::size_t Level>
        void run() const noexcept(noexcept(std::declval<Runner &>().run(
            static_cast<Node<Level> *>(nullptr), static_cast<NodeGroup<Level> *>(nullptr))))
        {
            auto &nodeGroupList = memoryManager->nodeGroupLists[Level];
            for(NodeGroupListEntry &nodeGroupListEntry : nodeGroupList)
            {
                auto *nodeGroupBase = nodeGroupListEntry.nodeGroup.get();
                assert(dynamic_cast<NodeGroup<Level> *>(nodeGroupBase));
                NodeGroup<Level> *nodeGroup = static_cast<NodeGroup<Level> *>(nodeGroupBase);
                nodeGroup->forEachAllocated([&](Node<Level> *node)
                                            {
                                                runner.run(node, nodeGroup);
                                            });
            }
        }
    };
    template <typename Runner>
    void runForEachNode(Runner &&runner) noexcept(
        noexcept(runForEachLevel(std::declval<RunForEachNode<Runner>>())))
    {
        runForEachLevel(RunForEachNode<Runner>(runner, this));
    }
    struct GCState final
    {
        std::array<std::deque<MemoryManagerNodeBase *>, LevelCount> worklists;
        std::default_random_engine randomEngine;
        explicit GCState(std::default_random_engine &globalRandomEngine)
            : worklists(), randomEngine()
        {
            randomEngine.seed(globalRandomEngine());
        }
    };
    template <std::size_t Level>
    static void visitNode(GCState &gcState, Node<Level> *node)
    {
        if(!node)
            return;
        if(node->gcMark)
            return;
        node->gcMark = true;
        gcState.worklists[Level].push_back(node);
    }
    struct GCRootVisitor final
    {
        GCState &gcState;
        constexpr explicit GCRootVisitor(GCState &gcState) noexcept : gcState(gcState)
        {
        }
        template <std::size_t Level>
        void operator()(Node<Level> *node) const
        {
            visitNode(gcState, node);
        }
        void operator()(std::nullptr_t) const noexcept
        {
        }
    };
    struct GCRoots
    {
        const DynamicNonowningNodeReference *roots;
        std::size_t rootCount;
        constexpr GCRoots(const DynamicNonowningNodeReference *roots, std::size_t rootCount) noexcept
            : roots(roots),
              rootCount(rootCount)
        {
        }
        void visit(GCState &gcState) const
        {
            for(std::size_t i = 0; i < rootCount; i++)
            {
                roots[i].dispatch(GCRootVisitor(gcState));
            }
        }
    };
    struct CollectGarbageRunTraceAndSweep
    {
        GCState &gcState;
        MemoryManager *memoryManager;
        constexpr CollectGarbageRunTraceAndSweep(GCState &gcState,
                                                 MemoryManager *memoryManager) noexcept
            : gcState(gcState),
              memoryManager(memoryManager)
        {
        }
        struct NodeVisitor final
        {
            GCState &gcState;
            constexpr explicit NodeVisitor(GCState &gcState) noexcept : gcState(gcState)
            {
            }
            template <std::size_t Level>
            bool operator()(Node<Level> *node) const
            {
                visitNode(gcState, node);
                return true;
            }
            bool operator()(Node<0>::KeyElement) const
            {
                return true;
            }
        };
        template <std::size_t LevelIn>
        void run() const
        {
            constexpr std::size_t level = LevelCount - 1 - LevelIn;
            static_assert(level < LevelCount, "");
            auto &worklist = gcState.worklists[level];
            std::default_random_engine randomEngine; // local for extra speed
            randomEngine.seed(gcState.randomEngine());
            while(!worklist.empty())
            {
                auto *nodeBase = worklist.back();
                worklist.pop_back();
                auto *node = static_cast<Node<level> *>(nodeBase);
                node->trace(NodeVisitor(gcState));
            }
            auto &nodeGroupList = memoryManager->nodeGroupLists[level];
            for(NodeGroupListEntry &nodeGroupListEntry : nodeGroupList)
            {
                auto *nodeGroupBase = nodeGroupListEntry.nodeGroup.get();
                assert(dynamic_cast<NodeGroup<level> *>(nodeGroupBase));
                NodeGroup<level> *nodeGroup = static_cast<NodeGroup<level> *>(nodeGroupBase);
                nodeGroup->forEachAllocated(
                    [&](Node<level> *node)
                    {
                        if(node->gcMark)
                            return;
                        bool keepNode = std::uniform_int_distribution<int>(0, 9)(randomEngine) >= 7;
                        if(keepNode)
                            node->trace(NodeVisitor(gcState)); // trace so we don't lose children
                        else
                            nodeGroup->free(node);
                    });
            }
        }
    };
    std::default_random_engine garbageCollectRandomEngine;
    void collectGarbage(const GCRoots &roots);

public:
    void collectGarbage(const DynamicNonowningNodeReference *roots, std::size_t rootCount)
    {
        collectGarbage(GCRoots(roots, rootCount));
    }
#warning implement allocate nodes function
};
}
}
}
}

#endif /* WORLD_PARALLEL_HASHLIFE_MEMORY_MANAGER_H_ */
