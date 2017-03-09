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
#include "vulkan_command_buffer.h"

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
VulkanCommandPool::VulkanCommandPool(std::shared_ptr<const VulkanDevice> device, PrivateAccess)
    : vk(device->vk), device(std::move(device)), commandPool(VK_NULL_HANDLE), commandPoolGood(false)
{
}

VulkanCommandPool::~VulkanCommandPool()
{
    if(commandPoolGood)
        vk->DestroyCommandPool(device->device, commandPool, nullptr);
}

std::shared_ptr<const VulkanCommandPool> VulkanCommandPool::make(
    std::shared_ptr<const VulkanDevice> device,
    bool transient,
    bool resettable,
    std::uint32_t queueFamilyIndex)
{
    auto retval = std::make_shared<VulkanCommandPool>(device, PrivateAccess());
    auto &vk = retval->vk;
    VkCommandPoolCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    if(transient)
        createInfo.flags |= VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
    if(resettable)
        createInfo.flags |= VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    createInfo.queueFamilyIndex = queueFamilyIndex;
    handleVulkanResult(vk->CreateCommandPool(device->device, &createInfo, nullptr, &retval->commandPool),
                       "vkCreateCommandPool");
    retval->commandPoolGood = true;
    return retval;
}

VulkanCommandBuffer::VulkanCommandBuffer(std::shared_ptr<const VulkanCommandPool> commandPool,
                                         PrivateAccess)
    : vk(commandPool->vk),
      commandPool(std::move(commandPool)),
      commandBuffer(VK_NULL_HANDLE),
      commandBufferGood(false)
{
}

VulkanCommandBuffer::~VulkanCommandBuffer()
{
    if(commandBufferGood)
        vk->FreeCommandBuffers(
            commandPool->device->device, commandPool->commandPool, 1, &commandBuffer);
}

std::shared_ptr<const VulkanCommandBuffer> VulkanCommandBuffer::make(
    std::shared_ptr<const VulkanCommandPool> commandPool, VkCommandBufferLevel level)
{
    auto retval = std::make_shared<VulkanCommandBuffer>(commandPool, PrivateAccess());
    auto &vk = retval->vk;
    VkCommandBufferAllocateInfo allocateInfo{};
    allocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocateInfo.commandPool = commandPool->commandPool;
    allocateInfo.level = level;
    allocateInfo.commandBufferCount = 1;
    handleVulkanResult(vk->AllocateCommandBuffers(
                           commandPool->device->device, &allocateInfo, &retval->commandBuffer),
                       "vkAllocateCommandBuffers");
    retval->commandBufferGood = true;
    return retval;
}
}
}
}
}
}
