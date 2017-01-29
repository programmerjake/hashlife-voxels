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
#include "memory_manager.h"

#if 0
#include <iostream>
#include <cstdlib>
#include <random>
#include <vector>
namespace programmerjake
{
namespace voxels
{
namespace util
{
namespace
{
struct TestBaseAllocator final
{
    struct BaseType final
    {
        std::size_t index;
        std::shared_ptr<std::vector<bool>> allocated;
        BaseType(std::size_t index, std::size_t size)
            : index(index), allocated(std::make_shared<std::vector<bool>>(size, false))
        {
        }
    };
    typedef std::size_t SizeType;
    std::size_t nextAllocation = 0;
    std::size_t freeCount = 0;
    std::size_t currentMemoryUsage = 0;
    std::size_t maxMemoryUsage = 0;
    std::size_t currentAllocationCount = 0;
    std::size_t maxAllocationCount = 0;
    void free(BaseType base) noexcept
    {
        freeCount++;
        currentAllocationCount--;
        currentMemoryUsage -= base.allocated->size();
        std::cout << "base.free(" << base.index << ")" << std::endl;
        for(bool v : *base.allocated)
            assert(!v);
    }
    BaseType allocate(SizeType size)
    {
        currentMemoryUsage += size;
        if(currentMemoryUsage > maxMemoryUsage)
            maxMemoryUsage = currentMemoryUsage;
        currentAllocationCount++;
        if(currentAllocationCount > maxAllocationCount)
            maxAllocationCount = currentAllocationCount;
        BaseType retval(nextAllocation++, size);
        std::cout << "base.allocate(" << size << ") -> " << retval.index << std::endl;
        return retval;
    }
    ~TestBaseAllocator()
    {
        assert(freeCount == nextAllocation);
        std::cout << "base.maxMemoryUsage = " << maxMemoryUsage << std::endl;
        std::cout << "base.maxAllocationCount = " << maxAllocationCount << std::endl;
    }
};
void test()
{
    std::cout << std::hex << std::uppercase;
    typedef MemoryManager<TestBaseAllocator, 1ULL << 16> MemMgr;
    MemMgr memoryManager;
    std::default_random_engine re;
    std::vector<MemMgr::AllocationReference> allocations;
    std::size_t currentMemoryUsage = 0;
    std::size_t maxMemoryUsage = 0;
    auto writeAllocation = [](const MemMgr::AllocationReference &allocation)
    {
        std::cout << "{base = " << allocation.getBase().index
                  << ", offset = " << allocation.getOffset() << ", size = " << allocation.getSize()
                  << "}";
    };
    for(std::size_t i = 50000; i > 0; i--)
    {
        if(!allocations.empty()
           && (i <= allocations.size() || std::uniform_int_distribution<int>(0, 4)(re) < 2))
        {
            auto allocationIterator =
                allocations.begin()
                + std::uniform_int_distribution<std::size_t>(0, allocations.size() - 1)(re);
            std::size_t size = allocationIterator->getSize();
            currentMemoryUsage -= size;
            std::size_t offset = allocationIterator->getOffset();
            auto base = allocationIterator->getBase();
            for(std::size_t i = 0; i < size; i++)
            {
                base.allocated->at(i + offset) = false;
            }
            std::cout << "free(";
            writeAllocation(*allocationIterator);
            std::cout << ")" << std::endl;
            allocations.erase(allocationIterator);
        }
        else
        {
            std::size_t allocationSize = static_cast<std::size_t>(1)
                                         << std::uniform_int_distribution<int>(0, 20)(re);
            allocations.push_back(memoryManager.allocate(allocationSize));
            std::cout << "allocate(" << allocationSize << ") -> ";
            writeAllocation(allocations.back());
            std::cout << std::endl;
            std::size_t size = allocations.back().getSize();
            std::size_t offset = allocations.back().getOffset();
            auto base = allocations.back().getBase();
            for(std::size_t i = 0; i < size; i++)
            {
                assert(!base.allocated->at(i + offset));
                base.allocated->at(i + offset) = true;
            }
            currentMemoryUsage += allocationSize;
            if(currentMemoryUsage > maxMemoryUsage)
                maxMemoryUsage = currentMemoryUsage;
        }
    }
    std::cout << "maxMemoryUsage = " << maxMemoryUsage << std::endl;
}

struct MemoryManagerTest final
{
    MemoryManagerTest()
    {
        test();
        std::exit(0);
    }
} memoryManagerTest;
}
}
}
}
#endif
