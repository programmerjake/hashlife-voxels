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

#ifndef GRAPHICS_DRIVERS_VULKAN_VULKAN_BUFFER_ALLOCATOR_H_
#define GRAPHICS_DRIVERS_VULKAN_VULKAN_BUFFER_ALLOCATOR_H_

#include "vulkan_functions.h"
#include "../../../util/memory_manager.h"

namespace programmerjake
{
namespace voxels
{
namespace graphics
{
namespace drivers
{
namespace vulkan
{
class VulkanBufferAllocator final
{
private:
    struct BaseAllocator final
    {
        const std::shared_ptr<const VulkanFunctions> vk;
        typedef VkDeviceSize SizeType;
        struct Allocation final
        {
            const std::shared_ptr<const VulkanFunctions> vk;
            explicit Allocation(std::shared_ptr<const VulkanFunctions> vk) noexcept : vk(std::move(vk))
            {
            }
            ~Allocation()
            {

            }
        };
        typedef Allocation *BaseType;
        void free(BaseType block) noexcept
        {
            delete block;
        }
        BaseType allocate(SizeType blockSize)
        {
        }
    };
    typedef util::MemoryManager<BaseAllocator, static_cast<std::uint64_t>(1) << 20 /* 1MB */>
        BaseMemoryManager;

public:
};
}
}
}
}
}

#endif /* GRAPHICS_DRIVERS_VULKAN_VULKAN_BUFFER_ALLOCATOR_H_ */
