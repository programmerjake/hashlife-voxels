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

#ifndef WORLD_PARALLEL_HASHLIFE_WORLD_H_
#define WORLD_PARALLEL_HASHLIFE_WORLD_H_

#include "parallel_hashlife_node.h"
#include "parallel_hashlife_memory_manager.h"
#include "../util/constexpr_assert.h"
#include <mutex>
#include <vector>
#include <atomic>
#include <memory>
#include <utility>

namespace programmerjake
{
namespace voxels
{
namespace world
{
namespace parallel_hashlife
{
template <std::size_t Level>
struct OwningNodeReference;

struct DynamicOwningNodeReference;

class WorldBase
{
    template <std::size_t Level>
    friend struct OwningNodeReference;
    friend struct DynamicOwningNodeReference;
    WorldBase(const WorldBase &) = delete;
    WorldBase &operator=(const WorldBase &) = delete;

private:
    struct OwningNodeReferenceState final
    {
        std::atomic_size_t referenceCount{0};
        bool syncIfFree() const noexcept
        {
            if(referenceCount.load(std::memory_order_relaxed) == 0)
            {
                std::atomic_thread_fence(std::memory_order_acquire); // sync with freeing thread
                return true;
            }
            return false;
        }
        void addReference() noexcept
        {
            referenceCount.fetch_add(1, std::memory_order_relaxed);
        }
        void removeReference() noexcept
        {
            referenceCount.fetch_sub(1, std::memory_order_release);
        }
    };
    struct NodeReferenceStatesChunk final
    {
        static constexpr std::size_t targetSizeInBytes = 1U << 12;
        static constexpr std::size_t chunkSize =
            sizeof(OwningNodeReferenceState) / targetSizeInBytes;
        static_assert(chunkSize > 0, "");
        OwningNodeReferenceState states[chunkSize];
    };
    class OwningNodeReferenceBase
    {
        friend class WorldBase;

    private:
        OwningNodeReferenceState *owningNodeReferenceState;

    private:
        constexpr explicit OwningNodeReferenceBase(
            OwningNodeReferenceState *owningNodeReferenceState) noexcept
            : owningNodeReferenceState(owningNodeReferenceState)
        {
        }

    public:
        constexpr OwningNodeReferenceBase() noexcept : owningNodeReferenceState(nullptr)
        {
        }
        OwningNodeReferenceBase(const OwningNodeReferenceBase &rt) noexcept
            : owningNodeReferenceState(rt.owningNodeReferenceState)
        {
            if(owningNodeReferenceState)
                owningNodeReferenceState->addReference();
        }
        OwningNodeReferenceBase(OwningNodeReferenceBase &&rt) noexcept
            : owningNodeReferenceState(rt.owningNodeReferenceState)
        {
            rt.owningNodeReferenceState = nullptr;
        }
        ~OwningNodeReferenceBase()
        {
            if(owningNodeReferenceState)
                owningNodeReferenceState->removeReference();
        }
        OwningNodeReferenceBase &operator=(OwningNodeReferenceBase rt) noexcept
        {
            using std::swap;
            swap(owningNodeReferenceState, rt.owningNodeReferenceState);
            return *this;
        }
    };
    class RootManager final
    {
        RootManager(const RootManager &) = delete;
        RootManager &operator=(const RootManager &) = delete;

    private:
        std::mutex lock{};
        std::vector<DynamicNonowningNodeReference> roots{};
        std::vector<std::unique_ptr<NodeReferenceStatesChunk>> rootReferenceStatesChunks{};
        OwningNodeReferenceState &getState(std::size_t index) noexcept
        {
            return rootReferenceStatesChunks[index / NodeReferenceStatesChunk::chunkSize]
                ->states[index % NodeReferenceStatesChunk::chunkSize];
        }
        std::size_t nextFreeIndex = 0;

    public:
        RootManager() = default;
        OwningNodeReferenceBase createNewOwningNodeReference(
            DynamicNonowningNodeReference nodeReference)
        {
            if(!nodeReference)
                return OwningNodeReferenceBase();
            std::unique_lock<std::mutex> lockIt(lock);
            for(std::size_t i = 0; i < roots.size(); i++)
            {
                auto &state = getState(nextFreeIndex);
                auto &root = roots[nextFreeIndex];
                if(++nextFreeIndex >= roots.size())
                    nextFreeIndex = 0;
                if(getState(nextFreeIndex).syncIfFree())
                {
                    root = nodeReference;
                    state.addReference();
                    return OwningNodeReferenceBase(&state);
                }
            }
            // expansion required
            nextFreeIndex = roots.size();
            std::size_t newSize = roots.size() * 2;
            if(newSize < NodeReferenceStatesChunk::chunkSize)
                newSize = NodeReferenceStatesChunk::chunkSize;
            constexprAssert(newSize % NodeReferenceStatesChunk::chunkSize == 0);
            roots.resize(newSize);
            std::size_t startingChunkIndex = rootReferenceStatesChunks.size();
            rootReferenceStatesChunks.resize(newSize / NodeReferenceStatesChunk::chunkSize);
            for(std::size_t i = startingChunkIndex; i < rootReferenceStatesChunks.size(); i++)
                rootReferenceStatesChunks[i].reset(new NodeReferenceStatesChunk);

            auto &state = getState(nextFreeIndex);
            auto &root = roots[nextFreeIndex];
            if(++nextFreeIndex >= roots.size())
                nextFreeIndex = 0;
            root = nodeReference;
            state.addReference();
            return OwningNodeReferenceBase(&state);
        }
        template <typename Fn>
        void callWithLockedRoots(Fn fn)
        {
            std::unique_lock<std::mutex> lockIt(lock);
            for(std::size_t i = 0; i < roots.size(); i++)
                if(getState(nextFreeIndex).syncIfFree())
                    roots[nextFreeIndex] = nullptr;
            const auto &constRoots = roots;
            fn(constRoots);
        }
    };

private:
    MemoryManager memoryManager;
    RootManager rootManager;

public:
    WorldBase() : memoryManager(), rootManager()
    {
    }
    void collectGarbage()
    {
        rootManager.callWithLockedRoots([&](const std::vector<DynamicNonowningNodeReference> &roots)
                                        {
                                            memoryManager.collectGarbage(roots.data(),
                                                                         roots.size());
                                        });
    }
};
}
}
}
}

#endif /* WORLD_PARALLEL_HASHLIFE_WORLD_H_ */
