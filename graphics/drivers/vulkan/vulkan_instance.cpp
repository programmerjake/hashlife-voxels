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
#include "vulkan_instance.h"
#include "wmhelper.h"

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
VulkanInstance::~VulkanInstance()
{
    if(vk->DestroyInstance && instanceGood)
        vk->DestroyInstance(instance);
}

VulkanInstance::VulkanInstance(std::shared_ptr<VulkanFunctions> vulkanFunctions,
                               const WMHelper *wmHelper,
                               PrivateAccess)
    : vk(std::move(vulkanFunctions)),
      instance(VK_NULL_HANDLE),
      instanceGood(false),
      physicalDevices(),
      wmHelper(wmHelper)
{
}

namespace
{
const WMHelper *findWorkingWMHelper(SDL_Window *window)
{
    for(auto *wmHelper : WMHelpers())
    {
        if(wmHelper->isUsableWithWindow(window))
            return wmHelper;
    }
    throw std::runtime_error("vulkan: no usable WMHelper");
}
}

std::shared_ptr<const VulkanInstance> VulkanInstance::make(std::shared_ptr<VulkanFunctions> vk,
                                                           SDL_Window *window)
{
    auto retval = std::make_shared<VulkanInstance>(vk, findWorkingWMHelper(window), PrivateAccess());
    VkApplicationInfo appInfo = {};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.apiVersion = VK_API_VERSION_1_0;
#warning give vulkan our application name and version
    VkInstanceCreateInfo instanceCreateInfo = {};
    instanceCreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instanceCreateInfo.pApplicationInfo = &appInfo;
    std::vector<const char *> enabledInstanceLayers;
#if 1
#warning add code to check for layers
#endif
    instanceCreateInfo.enabledLayerCount = enabledInstanceLayers.size();
    instanceCreateInfo.ppEnabledLayerNames = enabledInstanceLayers.data();
    auto enabledInstanceExtensions = retval->wmHelper->getWMExtensionNames(window);
    enabledInstanceExtensions.push_back(VK_KHR_SURFACE_EXTENSION_NAME);
    instanceCreateInfo.enabledExtensionCount = enabledInstanceExtensions.size();
    instanceCreateInfo.ppEnabledExtensionNames = enabledInstanceExtensions.data();
    handleVulkanResult(vk->CreateInstance(&instanceCreateInfo, nullptr, &retval->instance),
                       "vkCreateInstance");
    retval->instanceGood = true;
    vk->loadInstanceFunctions(retval->instance);
    std::uint32_t physicalDeviceCount = 0;
    handleVulkanResult(vk->EnumeratePhysicalDevices(*instance, &physicalDeviceCount, nullptr),
                       "vkEnumeratePhysicalDevices");
    if(physicalDeviceCount == 0)
        throw std::runtime_error("Vulkan: no physical devices");
    retval->physicalDevices.resize(physicalDeviceCount);
    handleVulkanResult(vk->EnumeratePhysicalDevices(
                           *instance, &physicalDeviceCount, retval->physicalDevices.data()),
                       "vkEnumeratePhysicalDevices");
    assert(retval->physicalDevices.size() == physicalDeviceCount);
    return retval;
}
}
}
}
}
}
