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
#include "vulkan_device.h"

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
VulkanDevice::VulkanDevice(std::shared_ptr<const VulkanInstance> instance,
                           std::shared_ptr<const VkSurfaceKHR> surface,
                           PrivateAccess)
    : vk(instance->vk),
      instance(std::move(instance)),
      surface(std::move(surface)),
      device(VK_NULL_HANDLE),
      deviceGood(false),
      queueFamiliesProperties()
{
}

VulkanDevice::~VulkanDevice()
{
    if(vk->DestroyDevice && deviceGood)
        vk->DestroyDevice(device, nullptr);
}

std::shared_ptr<const VulkanDevice> VulkanDevice::make(
    std::shared_ptr<const VulkanInstance> instance, std::shared_ptr<const VkSurfaceKHR> surface)
{
    auto retval = std::make_shared<VulkanDevice>(instance, surface, PrivateAccess());
    auto &vk = retval->vk;
    const std::string swapchainExtensionName = VK_KHR_SWAPCHAIN_EXTENSION_NAME;
    std::vector<VkExtensionProperties> deviceExtensions;
    for(VkPhysicalDevice physicalDevice : instance->physicalDevices)
    {
        vk->GetPhysicalDeviceProperties(physicalDevice, &retval->physicalDeviceProperties);
        if(VK_VERSION_MAJOR(retval->physicalDeviceProperties.apiVersion) < 1)
            continue;
        vk->GetPhysicalDeviceFeatures(physicalDevice, &retval->physicalDeviceFeatures);
        vk->GetPhysicalDeviceMemoryProperties(physicalDevice,
                                              &retval->physicalDeviceMemoryProperties);
        std::uint32_t queueFamiliesCount = 0;
        vk->GetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamiliesCount, nullptr);
        if(queueFamiliesCount == 0)
            continue;
        retval->queueFamiliesProperties.resize(queueFamiliesCount);
        vk->GetPhysicalDeviceQueueFamilyProperties(
            physicalDevice, &queueFamiliesCount, retval->queueFamiliesProperties.data());
        assert(retval->queueFamiliesProperties.size() == queueFamiliesCount);
        retval->graphicsQueueFamilyIndex = queueFamiliesCount;
        retval->presentQueueFamilyIndex = queueFamiliesCount;
        for(std::uint32_t i = 0; i < queueFamiliesCount; i++)
        {
            if(retval->queueFamiliesProperties[i].queueCount == 0)
                continue;
            if(retval->queueFamiliesProperties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
                retval->graphicsQueueFamilyIndex = i;
            VkBool32 supported = 0;
            handleVulkanResult(
                vk->GetPhysicalDeviceSurfaceSupportKHR(physicalDevice, i, *surface, &supported),
                "vkGetPhysicalDeviceSurfaceSupportKHR");
            if(supported)
            {
                retval->presentQueueFamilyIndex = i;
                if(retval->queueFamiliesProperties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
                    break; // use this queue because it can present and do graphics
            }
        }
        if(retval->graphicsQueueFamilyIndex == queueFamiliesCount) // no good queues found
            continue;
        if(retval->presentQueueFamilyIndex == queueFamiliesCount) // no good queues found
            continue;
        std::uint32_t deviceExtensionCount = 0;
        handleVulkanResult(vk->EnumerateDeviceExtensionProperties(
                               physicalDevice, nullptr, &deviceExtensionCount, nullptr),
                           "vkEnumerateDeviceExtensionProperties");
        deviceExtensions.resize(deviceExtensionCount);
        if(deviceExtensionCount > 0)
            handleVulkanResult(
                vk->EnumerateDeviceExtensionProperties(
                    physicalDevice, nullptr, &deviceExtensionCount, deviceExtensions.data()),
                "vkEnumerateDeviceExtensionProperties");
        bool hasSwapchainExtension = false;
        for(auto &deviceExtension : deviceExtensions)
        {
            if(deviceExtension.extensionName == swapchainExtensionName)
            {
                hasSwapchainExtension = true;
                break;
            }
        }
        if(!hasSwapchainExtension)
            continue;
        static const float queuePriority[] = {1.0f};
        VkDeviceQueueCreateInfo deviceQueueCreateInfo[2] = {};
        deviceQueueCreateInfo[0].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        deviceQueueCreateInfo[0].queueFamilyIndex = retval->graphicsQueueFamilyIndex;
        deviceQueueCreateInfo[0].queueCount = 1;
        deviceQueueCreateInfo[0].pQueuePriorities = &queuePriority[0];
        deviceQueueCreateInfo[1].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        deviceQueueCreateInfo[1].queueFamilyIndex = retval->presentQueueFamilyIndex;
        deviceQueueCreateInfo[1].queueCount = 1;
        deviceQueueCreateInfo[1].pQueuePriorities = &queuePriority[0];
        VkDeviceCreateInfo deviceCreateInfo{};
        deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        deviceCreateInfo.queueCreateInfoCount =
            retval->graphicsQueueFamilyIndex == retval->presentQueueFamilyIndex ? 1 : 2;
        deviceCreateInfo.pQueueCreateInfos = deviceQueueCreateInfo;
        deviceCreateInfo.pEnabledFeatures = nullptr;
        static const char *const deviceExtensionNames[] = {
            VK_KHR_SWAPCHAIN_EXTENSION_NAME,
        };
        deviceCreateInfo.enabledExtensionCount =
            sizeof(deviceExtensionNames) / sizeof(deviceExtensionNames[0]);
        deviceCreateInfo.ppEnabledExtensionNames = deviceExtensionNames;
        handleVulkanResult(
            vk->CreateDevice(physicalDevice, &deviceCreateInfo, nullptr, &retval->device),
            "vkCreateDevice");
        retval->deviceGood = true;
        vk->loadDeviceFunctions(retval->device);
        vk->GetDeviceQueue(
            retval->device, retval->graphicsQueueFamilyIndex, 0, &retval->graphicsQueue);
        vk->GetDeviceQueue(
            retval->device, retval->presentQueueFamilyIndex, 0, &retval->presentQueue);
        return retval;
    }
    throw std::runtime_error("Vulkan: no viable physical devices found");
}
}
}
}
}
}
