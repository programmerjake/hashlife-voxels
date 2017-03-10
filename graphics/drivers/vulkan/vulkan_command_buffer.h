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

#ifndef GRAPHICS_DRIVERS_VULKAN_VULKAN_COMMAND_BUFFER_H_
#define GRAPHICS_DRIVERS_VULKAN_VULKAN_COMMAND_BUFFER_H_

#include "vulkan_device.h"
#include <memory>

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
struct VulkanCommandPool final
{
    VulkanCommandPool(const VulkanCommandPool &) = delete;
    VulkanCommandPool &operator=(const VulkanCommandPool &) = delete;

private:
    struct PrivateAccess final
    {
        friend struct VulkanCommandPool;

    private:
        PrivateAccess() = default;
    };

public:
    const std::shared_ptr<const VulkanFunctions> vk;
    const std::shared_ptr<const VulkanDevice> device;
    VkCommandPool commandPool;
    bool commandPoolGood;
    VulkanCommandPool(std::shared_ptr<const VulkanDevice> device, PrivateAccess);
    ~VulkanCommandPool();
    static std::shared_ptr<const VulkanCommandPool> make(std::shared_ptr<const VulkanDevice> device,
                                                         bool transient,
                                                         bool resettable,
                                                         std::uint32_t queueFamilyIndex);
};

struct VulkanCommandBuffer final
{
    VulkanCommandBuffer(const VulkanCommandBuffer &) = delete;
    VulkanCommandBuffer &operator=(const VulkanCommandBuffer &) = delete;

private:
    struct PrivateAccess final
    {
        friend struct VulkanCommandBuffer;

    private:
        PrivateAccess() = default;
    };

public:
    const std::shared_ptr<const VulkanFunctions> vk;
    const std::shared_ptr<const VulkanCommandPool> commandPool;
    VkCommandBuffer commandBuffer;
    bool commandBufferGood;
    VulkanCommandBuffer(std::shared_ptr<const VulkanCommandPool> commandPool, PrivateAccess);
    ~VulkanCommandBuffer();
    static std::shared_ptr<const VulkanCommandBuffer> make(
        std::shared_ptr<const VulkanCommandPool> commandPool, VkCommandBufferLevel level);
};
}
}
}
}
}

#endif /* GRAPHICS_DRIVERS_VULKAN_VULKAN_COMMAND_BUFFER_H_ */