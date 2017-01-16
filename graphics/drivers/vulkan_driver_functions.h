/*
 * Copyright (C) 2012-2016 Jacob R. Lifshay
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

#ifndef VULKAN_DRIVER_GLOBAL_FUNCTION
#define VULKAN_DRIVER_GLOBAL_FUNCTION(x)
#endif
#ifndef VULKAN_DRIVER_INSTANCE_FUNCTION
#define VULKAN_DRIVER_INSTANCE_FUNCTION(x)
#endif
#ifndef VULKAN_DRIVER_DEVICE_FUNCTION
#define VULKAN_DRIVER_DEVICE_FUNCTION(x)
#endif

VULKAN_DRIVER_GLOBAL_FUNCTION(vkEnumerateInstanceLayerProperties)
VULKAN_DRIVER_GLOBAL_FUNCTION(vkEnumerateInstanceExtensionProperties)
VULKAN_DRIVER_GLOBAL_FUNCTION(vkCreateInstance)
VULKAN_DRIVER_INSTANCE_FUNCTION(vkEnumeratePhysicalDevices)
VULKAN_DRIVER_INSTANCE_FUNCTION(vkGetDeviceProcAddr)
VULKAN_DRIVER_INSTANCE_FUNCTION(vkGetPhysicalDeviceProperties)
VULKAN_DRIVER_INSTANCE_FUNCTION(vkGetPhysicalDeviceFeatures)
VULKAN_DRIVER_INSTANCE_FUNCTION(vkGetPhysicalDeviceQueueFamilyProperties)
VULKAN_DRIVER_INSTANCE_FUNCTION(vkCreateDevice)
VULKAN_DRIVER_INSTANCE_FUNCTION(vkEnumerateDeviceExtensionProperties)
#ifdef VK_USE_PLATFORM_XCB_KHR
VULKAN_DRIVER_INSTANCE_FUNCTION(vkCreateXcbSurfaceKHR)
#endif
VULKAN_DRIVER_DEVICE_FUNCTION(vkGetDeviceQueue)
VULKAN_DRIVER_DEVICE_FUNCTION(vkDeviceWaitIdle)

#undef VULKAN_DRIVER_GLOBAL_FUNCTION
#undef VULKAN_DRIVER_INSTANCE_FUNCTION
#undef VULKAN_DRIVER_DEVICE_FUNCTION
