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

#ifndef UTIL_MEMORY_MANAGER_H_
#define UTIL_MEMORY_MANAGER_H_

#if 0
#warning finish MemoryManager
#else
#include <memory>
#include <cstdint>
#include <atomic>
#include <utility>
#include <cstddef>
#include <list>
#include <forward_list>
#include <mutex>
#include <type_traits>
#include "spinlock.h"
#include "constexpr_assert.h"

namespace programmerjake
{
namespace voxels
{
namespace util
{
/**
 * @tparam BaseAllocator should have a structure similar to
 * @code
 * struct BaseAllocator
 * {
 *     typedef unspecified SizeType; // your unsigned integral size type; probably std::size_t
 *     typedef unspecified BaseType; // allocated block handle type; probably void *
 *     void free(BaseType block) noexcept; // free a block
 *     BaseType allocate(SizeType blockSize); // allocate a block with size blockSize
 * };
 * @endcode
 */
template <typename BaseAllocator,
          std::uint64_t BigChunkThreshold = static_cast<std::uint64_t>(1) << 12 /* 4k */,
          std::size_t Alignment = alignof(std::max_align_t),
          std::size_t AllocationGranularity = 1>
class MemoryManager final
{
    MemoryManager(const MemoryManager &) = delete;
    MemoryManager &operator=(const MemoryManager &) = delete;

public:
    typedef typename BaseAllocator::SizeType SizeType;
    static_assert(std::is_unsigned<SizeType>::value,
                  "SizeType needs to be an unsigned integer type");
    static_assert(static_cast<std::size_t>(-1) <= static_cast<SizeType>(-1),
                  "SizeType needs to be at least as big as std::size_t");
    static_assert(static_cast<std::size_t>(-1) <= static_cast<std::uint64_t>(-1),
                  "std::size_t can't be bigger than std::uint64_t");
    typedef typename BaseAllocator::BaseType BaseType;
    static_assert(std::is_nothrow_copy_assignable<BaseType>::value,
                  "BaseType needs to be nothrow copy assignable");
    static_assert(std::is_nothrow_copy_constructible<BaseType>::value,
                  "BaseType needs to be nothrow copy constructible");
    static_assert(std::is_nothrow_move_assignable<BaseType>::value,
                  "BaseType needs to be nothrow move assignable");
    static_assert(std::is_nothrow_move_constructible<BaseType>::value,
                  "BaseType needs to be nothrow move constructible");
    static_assert(std::is_nothrow_destructible<BaseType>::value,
                  "BaseType needs to be nothrow destructible");
    static constexpr std::size_t alignment = Alignment;
    static constexpr std::size_t allocationGranularity = AllocationGranularity;
    static constexpr std::uint64_t bigChunkThreshold = BigChunkThreshold;

private:
    static constexpr bool isPositivePowerOf2(std::uint64_t v) noexcept
    {
        return (v & (v - 1)) == 0 && v != 0;
    }
    static constexpr SizeType alignOffset(SizeType offset, SizeType newAlignment) noexcept
    {
        return (constexprAssert(isPositivePowerOf2(newAlignment)),
                (offset + (newAlignment - 1)) & ~(newAlignment - 1));
    }
    static constexpr std::size_t cachedChunkCount = 2;
    static constexpr std::uint64_t chunkSize = bigChunkThreshold * 4;
    static_assert(isPositivePowerOf2(bigChunkThreshold),
                  "bigChunkThreshold needs to be a positive power of 2");
    static_assert(isPositivePowerOf2(alignment) && alignment < bigChunkThreshold,
                  "alignment needs to be a positive power of 2 and less than bigChunkThreshold");
    static_assert(isPositivePowerOf2(allocationGranularity),
                  "allocationGranularity needs to be a positive power of 2");
    struct PrivateAccess final
    {
        friend class MemoryManager;

    private:
        PrivateAccess() = default;
    };
    struct Subchunk final
    {
        SizeType offset;
        SizeType size;
        bool free;
        constexpr Subchunk(SizeType offset, SizeType size, bool free) noexcept : offset(offset),
                                                                                 size(size),
                                                                                 free(free)
        {
        }
        constexpr bool isBigEnoughForNewSubchunk(SizeType newSize, SizeType newAlignment) const
            noexcept
        {
            return newSize + (alignOffset(offset, newAlignment) - offset) <= size;
        }
    };
    struct Chunk final
    {
        Chunk(const Chunk &) = delete;
        Chunk &operator=(const Chunk &) = delete;
        BaseType base;
        const SizeType totalSize;
        SizeType usedSize;
        std::list<Subchunk> subchunks;
        typename std::list<Subchunk>::iterator currentSubchunk;
        Spinlock chunkLock;
        const bool isBigChunk;
        explicit Chunk(BaseType base, SizeType totalSize, bool isBigChunk)
            : base(std::move(base)),
              totalSize(totalSize),
              usedSize(0),
              subchunks(),
              currentSubchunk(subchunks.begin()),
              chunkLock(),
              isBigChunk(isBigChunk)
        {
        }
    };
    struct Allocation final
    {
        Allocation(const Allocation &) = delete;
        Allocation &operator=(const Allocation &) = delete;
        typename std::list<Chunk>::iterator chunkIterator;
        typename std::list<Subchunk>::iterator subchunkIterator;
        const SizeType size;
        const SizeType offset;
        std::atomic_size_t referenceCount;
        MemoryManager *const memoryManager;
        static void incReference(Allocation *allocation) noexcept
        {
            if(allocation)
                allocation->referenceCount.fetch_add(1, std::memory_order_relaxed);
        }
        static void decReference(Allocation *allocation) noexcept
        {
            if(allocation
               && allocation->referenceCount.fetch_sub(1, std::memory_order_acq_rel) == 1)
                allocation->memoryManager->free(allocation);
        }
        Allocation(typename std::list<Chunk>::iterator chunkIterator,
                   typename std::list<Subchunk>::iterator subchunkIterator,
                   SizeType size,
                   SizeType offset,
                   std::size_t referenceCount,
                   MemoryManager *memoryManager) noexcept
            : chunkIterator(std::move(chunkIterator)),
              subchunkIterator(std::move(subchunkIterator)),
              size(size),
              offset(offset),
              referenceCount(referenceCount),
              memoryManager(memoryManager)
        {
        }
        SizeType getSize() const noexcept
        {
            return size;
        }
        SizeType getOffset() const noexcept
        {
            return offset;
        }
        BaseType getBase() const noexcept
        {
            return chunkIterator->base;
        }
    };

public:
    class AllocationReference final
    {
    private:
        Allocation *allocation;

    public:
        constexpr AllocationReference(Allocation *allocation, PrivateAccess) noexcept
            : allocation(allocation)
        {
        }
        constexpr AllocationReference() noexcept : allocation(nullptr)
        {
        }
        constexpr AllocationReference(std::nullptr_t) noexcept : allocation(nullptr)
        {
        }
        AllocationReference(const AllocationReference &rt) noexcept : allocation(rt.allocation)
        {
            Allocation::incReference(allocation);
        }
        AllocationReference(AllocationReference &&rt) noexcept : allocation(rt.allocation)
        {
            rt.allocation = nullptr;
        }
        void swap(AllocationReference &rt) noexcept
        {
            std::swap(allocation, rt.allocation);
        }
        AllocationReference &operator=(AllocationReference rt) noexcept
        {
            swap(rt);
            return *this;
        }
        ~AllocationReference()
        {
            Allocation::decReference(allocation);
        }
        explicit operator bool() const noexcept
        {
            return allocation != nullptr;
        }
        SizeType getSize() const noexcept
        {
            return allocation->getSize();
        }
        SizeType getOffset() const noexcept
        {
            return allocation->getOffset();
        }
        BaseType getBase() const noexcept
        {
            return allocation->getBase();
        }
        Allocation *get(PrivateAccess) const noexcept
        {
            return allocation;
        }
        Allocation *detach(PrivateAccess) noexcept
        {
            auto retval = allocation;
            allocation = nullptr;
            return retval;
        }
        friend bool operator==(std::nullptr_t, const AllocationReference &rt) noexcept
        {
            return rt.allocation == nullptr;
        }
        friend bool operator==(const AllocationReference &rt, std::nullptr_t) noexcept
        {
            return rt.allocation == nullptr;
        }
        friend bool operator!=(std::nullptr_t, const AllocationReference &rt) noexcept
        {
            return rt.allocation != nullptr;
        }
        friend bool operator!=(const AllocationReference &rt, std::nullptr_t) noexcept
        {
            return rt.allocation != nullptr;
        }
        bool operator==(const AllocationReference &rt) const noexcept
        {
            return allocation == rt.allocation;
        }
        bool operator!=(const AllocationReference &rt) const noexcept
        {
            return allocation != rt.allocation;
        }
    };

private:
    BaseAllocator baseAllocator;
    Spinlock
        chunksLock; // must not run lock operation while a chunk's lock is held to prevent deadlocks
    std::list<Chunk> chunks;
    std::list<Chunk> bigChunks;
    std::list<Chunk> freeChunks;
    typename std::list<Chunk>::iterator currentChunk;

private:
    void free(Allocation *allocation) noexcept
    {
        constexprAssert(allocation);
        constexprAssert(allocation->memoryManager == this);
        auto chunkIterator = allocation->chunkIterator;
        Chunk &chunk = *chunkIterator;
        auto subchunkIterator = allocation->subchunkIterator;
        Subchunk &subchunk = *subchunkIterator;
        delete allocation;
        std::unique_lock<Spinlock> globalLock(chunksLock);
        std::unique_lock<Spinlock> localLock(chunk.chunkLock);
        chunk.usedSize -= subchunk.size;
        if(chunk.usedSize != 0)
        {
            // we don't need to keep globalLock because we don't need to modify the lists of chunks
            globalLock.unlock();
        }
        subchunk.free = true;
        auto next = subchunkIterator;
        ++next;
        if(next != chunk.subchunks.end() && next->free)
        {
            subchunk.size += next->size;
            if(next == chunk.currentSubchunk)
                chunk.currentSubchunk = subchunkIterator;
            chunk.subchunks.erase(next);
        }
        if(subchunkIterator != chunk.subchunks.begin())
        {
            auto prev = subchunkIterator;
            --prev;
            if(prev->free)
            {
                prev->size += subchunk.size;
                if(subchunkIterator == chunk.currentSubchunk)
                    chunk.currentSubchunk = prev;
                chunk.subchunks.erase(subchunkIterator);
            }
        }
        if(chunk.usedSize == 0)
        {
            if(chunk.isBigChunk)
            {
                localLock.release(); // don't need to unlock because we are destroying immediately
                BaseType base = std::move(chunk.base);
                bigChunks.erase(chunkIterator);
                globalLock.unlock();
                baseAllocator.free(std::move(base));
            }
            else
            {
                if(currentChunk == chunkIterator)
                    ++currentChunk;
                freeChunks.splice(freeChunks.begin(), chunks, chunkIterator);
                if(freeChunks.size() > cachedChunkCount)
                {
                    BaseType base = std::move(freeChunks.back().base);
                    freeChunks.pop_back();
                    localLock.unlock();
                    globalLock.unlock();
                    baseAllocator.free(std::move(base));
                }
            }
        }
    }

public:
    template <typename... Args>
    explicit MemoryManager(Args &&... args)
        : baseAllocator(std::forward<Args>(args)...),
          chunksLock(),
          chunks(),
          bigChunks(),
          freeChunks(),
          currentChunk(chunks.end())
    {
    }
    ~MemoryManager()
    {
        constexprAssert(chunks.empty());
        constexprAssert(bigChunks.empty());
        shrink();
    }
    AllocationReference allocate(SizeType allocationSize)
    {
        constexprAssert(allocationSize != 0);
        allocationSize = (allocationSize + (allocationGranularity - 1))
                         & ~static_cast<SizeType>(allocationGranularity - 1);
        if(allocationSize >= bigChunkThreshold)
        {
            BaseType base = baseAllocator.allocate(allocationSize);
            try
            {
                std::list<Chunk> tempList;
                auto chunkIterator =
                    tempList.emplace(tempList.end(),
                                     base,
                                     allocationSize,
                                     true); // copy base so we can free it if this allocation fails
                Chunk &chunk = *chunkIterator;
                chunk.usedSize = allocationSize;
                auto subchunkIterator = chunk.subchunks.emplace(chunk.subchunks.end(),
                                                                Subchunk(0, allocationSize, false));
                chunk.currentSubchunk = chunk.subchunks.end();
                std::unique_ptr<Allocation> allocation(
                    new Allocation(chunkIterator, subchunkIterator, allocationSize, 0, 1, this));
                std::unique_lock<Spinlock> globalLock(chunksLock);
                bigChunks.splice(bigChunks.end(), tempList);
                globalLock.unlock();
                return AllocationReference(allocation.release(), PrivateAccess());
            }
            catch(...)
            {
                baseAllocator.free(std::move(base));
                throw;
            }
        }
        std::size_t allocationAlignment = alignment;
        if(allocationSize < alignment)
        {
            SizeType newSize = 1;
            while(newSize < allocationSize)
                newSize *= 2;
            allocationSize = newSize; // round up to next power of 2
            allocationAlignment = allocationSize;
        }
        std::unique_lock<Spinlock> globalLock(chunksLock);
        if(!chunks.empty())
        {
            if(currentChunk == chunks.end())
                currentChunk = chunks.begin();
            auto originalCurrentChunk = currentChunk;
            do
            {
                Chunk &chunk = *currentChunk;
                std::unique_lock<Spinlock> localLock(chunk.chunkLock);
                if((chunk.totalSize - chunk.usedSize) >= allocationSize)
                {
                    constexprAssert(!chunk.subchunks.empty());
                    if(chunk.currentSubchunk == chunk.subchunks.end())
                        chunk.currentSubchunk = chunk.subchunks.begin();
                    auto originalCurrentSubchunk = chunk.currentSubchunk;
                    do
                    {
                        Subchunk &subchunk = *chunk.currentSubchunk;
                        if(subchunk.free
                           && subchunk.isBigEnoughForNewSubchunk(allocationSize,
                                                                 allocationAlignment))
                        {
                            auto offset = alignOffset(subchunk.offset, allocationAlignment);
                            SizeType subchunkBeforeSize = offset - subchunk.offset;
                            SizeType subchunkAfterSize =
                                subchunk.size - allocationSize - subchunkBeforeSize;
                            // allocate everything beforehand so we don't have to recover from
                            // half-modified state
                            std::list<Subchunk> subchunkBeforeList;
                            auto subchunkBeforeIterator = subchunkBeforeList.end();
                            if(subchunkBeforeSize > 0)
                                subchunkBeforeIterator =
                                    subchunkBeforeList.emplace(subchunkBeforeList.end(),
                                                               subchunk.offset,
                                                               subchunkBeforeSize,
                                                               true);
                            std::list<Subchunk> subchunkAfterList;
                            auto subchunkAfterIterator = subchunkAfterList.end();
                            if(subchunkAfterSize > 0)
                                subchunkAfterIterator =
                                    subchunkAfterList.emplace(subchunkAfterList.end(),
                                                              offset + allocationSize,
                                                              subchunkAfterSize,
                                                              true);
                            std::unique_ptr<Allocation> allocation(
                                new Allocation(currentChunk,
                                               chunk.currentSubchunk,
                                               allocationSize,
                                               offset,
                                               1,
                                               this));
                            if(subchunkBeforeSize > 0)
                                chunk.subchunks.splice(chunk.currentSubchunk,
                                                       subchunkBeforeList,
                                                       subchunkBeforeIterator);
                            if(subchunkAfterSize > 0)
                            {
                                auto next = chunk.currentSubchunk;
                                ++next;
                                chunk.subchunks.splice(
                                    next, subchunkAfterList, subchunkAfterIterator);
                            }
                            subchunk.free = false;
                            subchunk.offset = offset;
                            subchunk.size = allocationSize;
                            chunk.usedSize += allocationSize;
                            localLock.unlock();
                            globalLock.unlock();
                            return AllocationReference(allocation.release(), PrivateAccess());
                        }
                        ++chunk.currentSubchunk;
                        if(chunk.currentSubchunk == chunk.subchunks.end())
                            chunk.currentSubchunk = chunk.subchunks.begin();
                    } while(originalCurrentSubchunk != chunk.currentSubchunk);
                }
                localLock.unlock();
                ++currentChunk;
                if(currentChunk == chunks.end())
                    currentChunk = chunks.begin();
            } while(originalCurrentChunk != currentChunk);
        }
        if(freeChunks.empty())
        {
            BaseType base = baseAllocator.allocate(chunkSize);
            try
            {
                std::list<Chunk> tempList;
                auto chunkIterator =
                    tempList.emplace(tempList.end(),
                                     base,
                                     chunkSize,
                                     false); // copy base so we can free it if this allocation fails
                Chunk &chunk = *chunkIterator;
                chunk.currentSubchunk =
                    chunk.subchunks.emplace(chunk.subchunks.end(), Subchunk(0, chunkSize, false));
                freeChunks.splice(freeChunks.end(), tempList);
            }
            catch(...)
            {
                baseAllocator.free(std::move(base));
                throw;
            }
        }
        auto chunkIterator = freeChunks.begin();
        Chunk &chunk = *chunkIterator;
        std::list<Subchunk> newFreeSubchunkList;
        auto newFreeSubchunkIterator = newFreeSubchunkList.emplace(
            newFreeSubchunkList.end(), allocationSize, chunk.totalSize - allocationSize, true);
        std::unique_ptr<Allocation> allocation(
            new Allocation(chunkIterator, chunk.subchunks.begin(), allocationSize, 0, 1, this));
        std::unique_lock<Spinlock> localLock(chunk.chunkLock);
        chunk.usedSize = allocationSize;
        chunk.subchunks.front() = Subchunk(0, allocationSize, false);
        chunk.subchunks.splice(chunk.subchunks.end(), newFreeSubchunkList, newFreeSubchunkIterator);
        chunk.currentSubchunk = newFreeSubchunkIterator;
        chunks.splice(chunks.begin(), freeChunks, chunkIterator);
        currentChunk = chunkIterator;
        localLock.unlock();
        globalLock.unlock();
        return AllocationReference(allocation.release(), PrivateAccess());
    }
    BaseAllocator &getBaseAllocator() noexcept
    {
        return baseAllocator;
    }
    const BaseAllocator &getBaseAllocator() const noexcept
    {
        return baseAllocator;
    }
    void shrink() noexcept
    {
        std::unique_lock<Spinlock> globalLock(chunksLock);
        std::size_t freeCount = freeChunks.size();
        while(!freeChunks.empty())
        {
            BaseType base = std::move(freeChunks.back().base);
            freeChunks.pop_back();
            globalLock.unlock();
            baseAllocator.free(std::move(base));
            if(--freeCount == 0)
                return;
            globalLock.lock();
        }
    }
};

template <typename BaseAllocator,
          std::uint64_t BigChunkThreshold,
          std::size_t Alignment,
          std::size_t AllocationGranularity>
constexpr std::size_t
    MemoryManager<BaseAllocator, BigChunkThreshold, Alignment, AllocationGranularity>::alignment;
template <typename BaseAllocator,
          std::uint64_t BigChunkThreshold,
          std::size_t Alignment,
          std::size_t AllocationGranularity>
constexpr std::size_t MemoryManager<BaseAllocator,
                                    BigChunkThreshold,
                                    Alignment,
                                    AllocationGranularity>::allocationGranularity;
template <typename BaseAllocator,
          std::uint64_t BigChunkThreshold,
          std::size_t Alignment,
          std::size_t AllocationGranularity>
constexpr std::uint64_t MemoryManager<BaseAllocator,
                                      BigChunkThreshold,
                                      Alignment,
                                      AllocationGranularity>::bigChunkThreshold;
template <typename BaseAllocator,
          std::uint64_t BigChunkThreshold,
          std::size_t Alignment,
          std::size_t AllocationGranularity>
constexpr std::size_t MemoryManager<BaseAllocator,
                                    BigChunkThreshold,
                                    Alignment,
                                    AllocationGranularity>::cachedChunkCount;
template <typename BaseAllocator,
          std::uint64_t BigChunkThreshold,
          std::size_t Alignment,
          std::size_t AllocationGranularity>
constexpr std::uint64_t
    MemoryManager<BaseAllocator, BigChunkThreshold, Alignment, AllocationGranularity>::chunkSize;
}
}
}
#endif

#endif /* UTIL_MEMORY_MANAGER_H_ */
