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
#include "vulkan_device.h"
#include <memory>
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
class VulkanBufferAllocator final : public std::enable_shared_from_this<VulkanBufferAllocator>
{
private:
    struct BaseAllocator final
    {
        const std::shared_ptr<const VulkanFunctions> vk;
        const std::shared_ptr<const VulkanDevice> device;
        const VkBufferUsageFlags usage;
        const std::vector<VkMemoryPropertyFlags> requiredPropertiesList;
        VulkanBufferAllocator &vulkanBufferAllocator;
        BaseAllocator(std::shared_ptr<const VulkanDevice> device,
                      VkBufferUsageFlags usage,
                      std::vector<VkMemoryPropertyFlags> requiredPropertiesList,
                      VulkanBufferAllocator &vulkanBufferAllocator)
            : vk(device->vk),
              device(std::move(device)),
              usage(usage),
              requiredPropertiesList(std::move(requiredPropertiesList)),
              vulkanBufferAllocator(vulkanBufferAllocator)
        {
        }
        typedef VkDeviceSize SizeType;
        struct Allocation final
        {
            BaseAllocator &baseAllocator;
            VkBuffer buffer;
            bool bufferGood;
            VkDeviceMemory deviceMemory;
            bool deviceMemoryGood;
            void *mappedMemory;
            bool mappedMemoryGood;
            // keep shared_ptr so the VulkanBufferAllocator isn't destroyed until the last
            // allocation is freed
            std::shared_ptr<VulkanBufferAllocator> vulkanBufferAllocator;
            std::uint32_t memoryTypeIndex;
            VkMemoryPropertyFlags memoryRequiredProperties;
            explicit Allocation(BaseAllocator &baseAllocator) noexcept
                : baseAllocator(baseAllocator),
                  buffer(VK_NULL_HANDLE),
                  bufferGood(false),
                  deviceMemory(VK_NULL_HANDLE),
                  deviceMemoryGood(false),
                  mappedMemory(nullptr),
                  mappedMemoryGood(false),
                  vulkanBufferAllocator(baseAllocator.vulkanBufferAllocator.shared_from_this()),
                  memoryTypeIndex(),
                  memoryRequiredProperties()
            {
            }
            ~Allocation()
            {
                auto &vk = baseAllocator.vk;
                if(mappedMemoryGood)
                    vk->UnmapMemory(baseAllocator.device->device, deviceMemory);
                if(bufferGood)
                    vk->DestroyBuffer(baseAllocator.device->device, buffer, nullptr);
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
            VkBufferCreateInfo createInfo{};
            createInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
            createInfo.size = blockSize;
            createInfo.usage = usage;
            createInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE; // only one queue
            createInfo.queueFamilyIndexCount = 0;
            createInfo.pQueueFamilyIndices = nullptr;
            handleVulkanResult(
                vk->CreateBuffer(device->device, &createInfo, nullptr, &retval->buffer),
                "vkCreateBuffer");
            retval->bufferGood = true;
            VkMemoryRequirements memoryRequirements;
            vk->GetBufferMemoryRequirements(device->device, retval->buffer, &memoryRequirements);
            std::uint32_t memoryTypeIndex = 0;
            VkMemoryPropertyFlags memoryRequiredProperties{};
            bool foundMemoryType = false;
            for(VkMemoryPropertyFlags requiredProperties : requiredPropertiesList)
            {
                auto findResult =
                    device->findMemoryType(memoryRequirements.memoryTypeBits, requiredProperties);
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
            allocateInfo.allocationSize = memoryRequirements.size;
            allocateInfo.memoryTypeIndex = memoryTypeIndex;
            handleVulkanResult(
                vk->AllocateMemory(device->device, &allocateInfo, nullptr, &retval->deviceMemory),
                "vkAllocateMemory");
            retval->deviceMemoryGood = true;
            handleVulkanResult(
                vk->BindBufferMemory(device->device, retval->buffer, retval->deviceMemory, 0),
                "vkBindBufferMemory");
            if(memoryRequiredProperties & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT)
            {
                handleVulkanResult(vk->MapMemory(device->device,
                                                 retval->deviceMemory,
                                                 0,
                                                 VK_WHOLE_SIZE,
                                                 0,
                                                 &retval->mappedMemory),
                                   "vkMapMemory");
                retval->mappedMemoryGood = true;
            }
            return retval.release();
        }
    };
    struct PrivateAccess final
    {
        friend class VulkanBufferAllocator;

    private:
        PrivateAccess() = default;
    };

private:
    typedef util::MemoryManager<BaseAllocator,
                                static_cast<std::uint64_t>(1) << 20 /* 1MB */,
                                0x100 /* max nonCoherentAtomSize from Vulkan spec */>
        MemoryManagerType;

private:
    MemoryManagerType memoryManager;

public:
    VulkanBufferAllocator(std::shared_ptr<const VulkanDevice> &&device,
                          VkBufferUsageFlags usage,
                          std::vector<VkMemoryPropertyFlags> &&requiredPropertiesList,
                          PrivateAccess)
        : memoryManager(std::move(device), usage, std::move(requiredPropertiesList), *this)
    {
    }
    static std::shared_ptr<VulkanBufferAllocator> make(
        std::shared_ptr<const VulkanDevice> device,
        VkBufferUsageFlags usage,
        std::vector<VkMemoryPropertyFlags> requiredPropertiesList)
    {
        return std::make_shared<VulkanBufferAllocator>(
            std::move(device), usage, std::move(requiredPropertiesList), PrivateAccess());
    }
    struct AllocationReference final
    {
    private:
        MemoryManagerType::AllocationReference allocationReference;

    public:
        constexpr AllocationReference() noexcept : allocationReference(nullptr)
        {
        }
        constexpr AllocationReference(std::nullptr_t) noexcept : allocationReference(nullptr)
        {
        }
        AllocationReference(MemoryManagerType::AllocationReference allocationReference,
                            PrivateAccess) noexcept
            : allocationReference(std::move(allocationReference))
        {
        }
        void swap(AllocationReference &rt) noexcept
        {
            allocationReference.swap(rt.allocationReference);
        }
        explicit operator bool() const noexcept
        {
            return static_cast<bool>(allocationReference);
        }
        VkDeviceSize getSize() const noexcept
        {
            return allocationReference.getSize();
        }
        VkDeviceSize getBufferOffset() const noexcept
        {
            return allocationReference.getOffset();
        }
        VkBuffer getBuffer() const noexcept
        {
            return allocationReference.getBase()->buffer;
        }
        void *getMappedMemory() const noexcept
        {
            auto *base = allocationReference.getBase();
            if(base->mappedMemoryGood)
                return static_cast<void *>(static_cast<char *>(base->mappedMemory)
                                           + allocationReference.getOffset());
            return nullptr;
        }
        std::uint32_t getMemoryTypeIndex() const noexcept
        {
            return allocationReference.getBase()->memoryTypeIndex;
        }
        VkMemoryPropertyFlags getMemoryRequiredProperties() const noexcept
        {
            return allocationReference.getBase()->memoryRequiredProperties;
        }
        friend bool operator==(std::nullptr_t, const AllocationReference &rt) noexcept
        {
            return rt.allocationReference == nullptr;
        }
        friend bool operator==(const AllocationReference &rt, std::nullptr_t) noexcept
        {
            return rt.allocationReference == nullptr;
        }
        friend bool operator!=(std::nullptr_t, const AllocationReference &rt) noexcept
        {
            return rt.allocationReference != nullptr;
        }
        friend bool operator!=(const AllocationReference &rt, std::nullptr_t) noexcept
        {
            return rt.allocationReference != nullptr;
        }
        bool operator==(const AllocationReference &rt) const noexcept
        {
            return allocationReference == rt.allocationReference;
        }
        bool operator!=(const AllocationReference &rt) const noexcept
        {
            return allocationReference != rt.allocationReference;
        }
    };
    void shrink() noexcept
    {
        memoryManager.shrink();
    }
    AllocationReference allocate(VkDeviceSize size)
    {
        return AllocationReference(memoryManager.allocate(size), PrivateAccess());
    }
};
}
}
}
}
}

#endif /* GRAPHICS_DRIVERS_VULKAN_VULKAN_BUFFER_ALLOCATOR_H_ */
