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

#ifndef GRAPHICS_DRIVERS_VULKAN_VULKAN_IMAGE_LAYER_ALLOCATOR_H_
#define GRAPHICS_DRIVERS_VULKAN_VULKAN_IMAGE_LAYER_ALLOCATOR_H_

#include "vulkan_image_allocator.h"
#include "../../../util/memory_manager.h"
#include <vector>

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
class VulkanImageLayerAllocator final
{
    VulkanImageLayerAllocator(const VulkanImageLayerAllocator &) = delete;
    VulkanImageLayerAllocator &operator=(const VulkanImageLayerAllocator &) = delete;

private:
    struct PrivateAccess final
    {
        friend class VulkanImageLayerAllocator;

    private:
        PrivateAccess() = default;
    };

private:
    static std::uint32_t highestSetBit(std::uint32_t v) noexcept
    {
        v |= v >> 1;
        v |= v >> 2;
        v |= v >> 4;
        v |= v >> 8;
        v |= v >> 16;
        return v & ~(v >> 1);
    }
    static std::uint32_t getLayerCountForImageFormat(
        const std::shared_ptr<const VulkanDevice> &device)
    {
        auto &vk = device->vk;
    }

private:
    struct BaseAllocator final
    {
        VulkanImageAllocator imageAllocator;
        VkImageCreateFlags flags;
        VkImageType imageType;
        std::uint32_t width;
        std::uint32_t height;
        std::uint32_t depth;
        uint32_t mipLevels;
        VkSampleCountFlagBits samples;
        VkImageUsageFlags usage;
        VkSharingMode sharingMode;
        std::vector<uint32_t> queueFamilyIndices;
        VkImageLayout initialLayout;
        typedef std::uint32_t SizeType;
        typedef std::shared_ptr<Image> *BaseType;
        void free(BaseType block) noexcept
        {
            delete block;
        }
        BaseType allocate(SizeType blockSize)
        {
            std::unique_ptr<std::shared_ptr<Image>> retval(
                new std::shared_ptr<Image>(imageAllocator.allocate(flags,
                                                                   imageType,
                                                                   width,
                                                                   height,
                                                                   depth,
                                                                   mipLevels,
                                                                   blockSize,
                                                                   samples,
                                                                   usage,
                                                                   sharingMode,
                                                                   queueFamilyIndices.size(),
                                                                   queueFamilyIndices.data(),
                                                                   initialLayout)));
#error transition layout to
            return retval.release();
        }
    };
    struct Image final
    {
    };
    typedef util::MemoryManager<BaseAllocator> MemoryManager;

private:
    MemoryManager memoryManager;

public:
    struct AllocationReference final
    {
    private:
        typename MemoryManager::AllocationReference allocationReference;

    public:
        explicit AllocationReference(PrivateAccess)
        {
        }
        std::shared_ptr<Image> getImage() const noexcept
        {
            return *allocationReference.getBase();
        }
    };
};
}
}
}
}
}

#endif /* GRAPHICS_DRIVERS_VULKAN_VULKAN_IMAGE_LAYER_ALLOCATOR_H_ */
