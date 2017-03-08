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

#ifndef GRAPHICS_DRIVERS_VULKAN_VULKAN_DEVICE_H_
#define GRAPHICS_DRIVERS_VULKAN_VULKAN_DEVICE_H_

#include "vulkan_instance.h"
#include <cstdint>

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
struct VulkanDevice final
{
private:
    struct PrivateAccess final
    {
        friend struct VulkanDevice;

    private:
        PrivateAccess() = default;
    };

public:
    std::shared_ptr<VulkanFunctions> vk;
    std::shared_ptr<const VulkanInstance> instance;
    std::shared_ptr<const VkSurfaceKHR> surface;
    VkDevice device;
    bool deviceGood;
    VkPhysicalDeviceProperties physicalDeviceProperties{};
    VkPhysicalDeviceFeatures physicalDeviceFeatures{};
    VkPhysicalDeviceMemoryProperties physicalDeviceMemoryProperties{};
    std::vector<VkQueueFamilyProperties> queueFamiliesProperties;
    std::uint32_t graphicsQueueFamilyIndex{};
    VkQueue graphicsQueue = VK_NULL_HANDLE;
    std::uint32_t presentQueueFamilyIndex{};
    VkQueue presentQueue = VK_NULL_HANDLE;
    ~VulkanDevice();
    explicit VulkanDevice(std::shared_ptr<const VulkanInstance> instance,
                          std::shared_ptr<const VkSurfaceKHR> surface,
                          PrivateAccess);
    static std::shared_ptr<const VulkanDevice> make(std::shared_ptr<const VulkanInstance> instance,
                                                    std::shared_ptr<const VkSurfaceKHR> surface);
};
}
}
}
}
}

#endif /* GRAPHICS_DRIVERS_VULKAN_VULKAN_DEVICE_H_ */
