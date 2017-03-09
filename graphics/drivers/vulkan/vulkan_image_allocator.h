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

#ifndef GRAPHICS_DRIVERS_VULKAN_VULKAN_IMAGE_ALLOCATOR_H_
#define GRAPHICS_DRIVERS_VULKAN_VULKAN_IMAGE_ALLOCATOR_H_

#include "vulkan_functions.h"
#include "../../../util/memory_manager.h"
#include "vulkan_device.h"
#include <memory>
#include <type_traits>
#include "../../../logging/logging.h"

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
class VulkanImageAllocator final : public std::enable_shared_from_this<VulkanImageAllocator>
{
private:
    struct BaseAllocator final
    {
        const std::shared_ptr<const VulkanFunctions> vk;
        const std::shared_ptr<const VulkanDevice> device;
        const VkFormat format;
        const VkImageTiling tiling;
        const std::vector<VkMemoryPropertyFlags> requiredPropertiesList;
        VulkanImageAllocator &vulkanImageAllocator;
        std::atomic_uint_least32_t memoryTypeBitsAtomic{0};
        BaseAllocator(std::shared_ptr<const VulkanDevice> device,
                      VkFormat format,
                      VkImageTiling tiling,
                      std::vector<VkMemoryPropertyFlags> requiredPropertiesList,
                      VulkanImageAllocator &vulkanImageAllocator)
            : vk(device->vk),
              device(std::move(device)),
              format(format),
              tiling(tiling),
              requiredPropertiesList(std::move(requiredPropertiesList)),
              vulkanImageAllocator(vulkanImageAllocator)
        {
        }
        typedef VkDeviceSize SizeType;
        struct Allocation final
        {
            BaseAllocator &baseAllocator;
            VkDeviceMemory deviceMemory;
            bool deviceMemoryGood;
            // keep shared_ptr so the VulkanImageAllocator isn't destroyed until the
            // last
            // allocation is freed
            std::shared_ptr<VulkanImageAllocator> vulkanImageAllocator;
            std::uint32_t memoryTypeIndex;
            VkMemoryPropertyFlags memoryRequiredProperties;
            explicit Allocation(BaseAllocator &baseAllocator) noexcept
                : baseAllocator(baseAllocator),
                  deviceMemory(VK_NULL_HANDLE),
                  deviceMemoryGood(false),
                  vulkanImageAllocator(baseAllocator.vulkanImageAllocator.shared_from_this()),
                  memoryTypeIndex(),
                  memoryRequiredProperties()
            {
            }
            ~Allocation()
            {
                auto &vk = baseAllocator.vk;
                if(deviceMemoryGood)
                    vk->FreeMemory(baseAllocator.device->device, deviceMemory, nullptr);
            }
        };
        typedef Allocation *BaseType;
        void free(BaseType block) noexcept
        {
            delete block;
        }
        BaseType allocate(SizeType blockSize)
        {
            auto retval = std::unique_ptr<Allocation>(new Allocation(*this));
            assert(blockSize);
            std::uint32_t memoryTypeIndex = 0;
            VkMemoryPropertyFlags memoryRequiredProperties{};
            bool foundMemoryType = false;
            std::uint32_t memoryTypeBits = memoryTypeBitsAtomic.load(std::memory_order_relaxed);
            for(VkMemoryPropertyFlags requiredProperties : requiredPropertiesList)
            {
                auto findResult = device->findMemoryType(memoryTypeBits, requiredProperties);
                if(std::get<1>(findResult))
                {
                    foundMemoryType = true;
                    memoryTypeIndex = std::get<0>(findResult);
                    retval->memoryTypeIndex = memoryTypeIndex;
                    memoryRequiredProperties = requiredProperties;
                    retval->memoryRequiredProperties = memoryRequiredProperties;
                    break;
                }
            }
            if(!foundMemoryType)
            {
                std::string message = "compatible memory type not found for buffer";
                logging::log(logging::Level::Fatal, "VulkanDriver", message);
                throw std::runtime_error("VulkanDriver: " + std::move(message));
            }
            VkMemoryAllocateInfo allocateInfo{};
            allocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
            allocateInfo.allocationSize = blockSize;
            allocateInfo.memoryTypeIndex = memoryTypeIndex;
            handleVulkanResult(
                vk->AllocateMemory(device->device, &allocateInfo, nullptr, &retval->deviceMemory),
                "vkAllocateMemory");
            retval->deviceMemoryGood = true;
            return retval.release();
        }
    };
    struct PrivateAccess final
    {
        friend class VulkanImageAllocator;

    private:
        PrivateAccess() = default;
    };

private:
    typedef util::MemoryManager<BaseAllocator,
                                static_cast<std::uint64_t>(8) << 20 /* 8MB */,
                                0x20000 /* max bufferImageGranularity from Vulkan spec */>
        MemoryManagerType;

private:
    MemoryManagerType memoryManager;

public:
    VulkanImageAllocator(std::shared_ptr<const VulkanDevice> &&device,
                         VkFormat format,
                         VkImageTiling tiling,
                         std::vector<VkMemoryPropertyFlags> &&requiredPropertiesList,
                         PrivateAccess)
        : memoryManager(std::move(device), format, tiling, std::move(requiredPropertiesList), *this)
    {
    }
    static std::shared_ptr<VulkanImageAllocator> make(
        std::shared_ptr<const VulkanDevice> device,
        VkFormat format,
        VkImageTiling tiling,
        std::vector<VkMemoryPropertyFlags> requiredPropertiesList)
    {
        return std::make_shared<VulkanImageAllocator>(
            std::move(device), format, tiling, std::move(requiredPropertiesList), PrivateAccess());
    }
    struct Image final
    {
        friend class VulkanImageAllocator;
        Image(const Image &) = delete;
        Image &operator=(const Image &) = delete;

    private:
        typename MemoryManagerType::AllocationReference allocationReference;
        VulkanImageAllocator *const vulkanImageAllocator;
        VkImage image;
        bool imageGood;

    public:
        Image(VulkanImageAllocator *vulkanImageAllocator, PrivateAccess) noexcept
            : allocationReference(),
              vulkanImageAllocator(vulkanImageAllocator),
              image(VK_NULL_HANDLE),
              imageGood(false)
        {
        }
        ~Image()
        {
            auto &baseAllocator = vulkanImageAllocator->memoryManager.getBaseAllocator();
            if(imageGood)
                baseAllocator.vk->DestroyImage(baseAllocator.device->device, image, nullptr);
        }
        VkImage getImage() const noexcept
        {
            return image;
        }
        std::uint32_t getMemoryTypeIndex() const noexcept
        {
            return allocationReference.getBase()->memoryTypeIndex;
        }
        VkMemoryPropertyFlags getMemoryRequiredProperties() const noexcept
        {
            return allocationReference.getBase()->memoryRequiredProperties;
        }
    };
    void shrink() noexcept
    {
        memoryManager.shrink();
    }
    std::shared_ptr<const Image> allocate(VkImageCreateFlags flags,
                                          VkImageType imageType,
                                          std::uint32_t width,
                                          std::uint32_t height,
                                          std::uint32_t depth,
                                          uint32_t mipLevels,
                                          uint32_t arrayLayers,
                                          VkSampleCountFlagBits samples,
                                          VkImageUsageFlags usage,
                                          VkSharingMode sharingMode,
                                          uint32_t queueFamilyIndexCount,
                                          const uint32_t *pQueueFamilyIndices,
                                          VkImageLayout initialLayout)
    {
        assert((flags & VK_IMAGE_CREATE_SPARSE_BINDING_BIT) == 0);
        assert((usage & VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT) == 0);
        auto &vk = memoryManager.getBaseAllocator().vk;
        auto retval = std::make_shared<Image>(this, PrivateAccess());
        VkImageCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        createInfo.flags = flags;
        createInfo.imageType = imageType;
        createInfo.format = memoryManager.getBaseAllocator().format;
        createInfo.extent.width = width;
        createInfo.extent.height = height;
        createInfo.extent.depth = depth;
        createInfo.mipLevels = mipLevels;
        createInfo.arrayLayers = arrayLayers;
        createInfo.samples = samples;
        createInfo.tiling = memoryManager.getBaseAllocator().tiling;
        createInfo.usage = usage;
        createInfo.sharingMode = sharingMode;
        createInfo.queueFamilyIndexCount = queueFamilyIndexCount;
        createInfo.pQueueFamilyIndices = pQueueFamilyIndices;
        createInfo.initialLayout = initialLayout;
        handleVulkanResult(vk->CreateImage(memoryManager.getBaseAllocator().device->device,
                                           &createInfo,
                                           nullptr,
                                           &retval->image),
                           "vkCreateImage");
        retval->imageGood = true;
        VkMemoryRequirements memoryRequirements;
        vk->GetImageMemoryRequirements(
            memoryManager.getBaseAllocator().device->device, retval->image, &memoryRequirements);
        auto &memoryTypeBitsAtomic = memoryManager.getBaseAllocator().memoryTypeBitsAtomic;
        std::uint32_t previousMemoryTypeBits = memoryTypeBitsAtomic.load(std::memory_order_relaxed);
        if(previousMemoryTypeBits == 0)
            memoryTypeBitsAtomic.store(memoryRequirements.memoryTypeBits,
                                       std::memory_order_relaxed);
        assert(previousMemoryTypeBits == 0
               || previousMemoryTypeBits == memoryRequirements.memoryTypeBits);
        retval->allocationReference =
            memoryManager.allocate(memoryRequirements.size, memoryRequirements.alignment);
        handleVulkanResult(vk->BindImageMemory(memoryManager.getBaseAllocator().device->device,
                                               retval->image,
                                               retval->allocationReference.getBase()->deviceMemory,
                                               retval->allocationReference.getOffset()),
                           "vkBindImageMemory");
        return retval;
    }
};
}
}
}
}
}

#endif /* GRAPHICS_DRIVERS_VULKAN_VULKAN_IMAGE_ALLOCATOR_H_ */
