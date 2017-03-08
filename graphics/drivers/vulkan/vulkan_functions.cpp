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
#include "vulkan_functions.h"
#include "../sdl2_driver.h"

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
void VulkanFunctions::handleFunctionLoadError(const char *name)
{
    throw std::runtime_error(std::string("can't load vulkan function: ") + name);
}

VulkanFunctions::VulkanFunctions(std::shared_ptr<void> vulkanLibraryIn, PrivateAccess)
    : vulkanLibrary(std::move(vulkanLibraryIn)), GetInstanceProcAddr()
{
    assert(vulkanLibrary);
#ifdef USE_PLATFORM_SDL_VULKAN
    GetInstanceProcAddr =
        reinterpret_cast<PFN_vkGetInstanceProcAddr>(SDL_Vulkan_GetVkGetInstanceProcAddr());
#else
    GetInstanceProcAddr = reinterpret_cast<PFN_vkGetInstanceProcAddr>(
        SDL_LoadFunction(vulkanLibrary.get(), "vkGetInstanceProcAddr"));
#endif
    if(!GetInstanceProcAddr)
        throw std::runtime_error("invalid vulkan loader: vkGetInstanceProcAddr not found");
    loadGlobalFunctions();
}

std::shared_ptr<VulkanFunctions> VulkanFunctions::loadVulkanLibrary()
{
#ifdef USE_PLATFORM_SDL_VULKAN
    SDL2Driver::initSDL();
    struct VulkanLibrary final
    {
        VulkanLibrary(const VulkanLibrary &) = delete;
        VulkanLibrary &operator=(const VulkanLibrary &) = delete;
        VulkanLibrary()
        {
            if(SDL_Vulkan_LoadLibrary(nullptr) != 0)
                throw std::runtime_error(std::string("SDL_Vulkan_LoadLibrary failed: ")
                                         + SDL_GetError());
        }
        ~VulkanLibrary()
        {
            SDL_Vulkan_UnloadLibrary();
        }
    };
    auto vulkanLibrary = std::make_shared<VulkanLibrary>();
    return std::make_shared<VulkanFunctions>(std::move(vulkanLibrary), PrivateAccess());
#else
#if defined(VK_USE_PLATFORM_XCB_KHR) || defined(VK_USE_PLATFORM_XLIB_KHR) \
    || defined(VK_USE_PLATFORM_MIR_KHR) || defined(VK_USE_PLATFORM_WAYLAND_KHR)
    const char *vulkanLoaderLibraryName = "libvulkan.so.1";
#elif defined(VK_USE_PLATFORM_WIN32_KHR)
    const char *vulkanLoaderLibraryName = "vulkan-1.dll";
#elif defined(VK_USE_PLATFORM_ANDROID_KHR)
    const char *vulkanLoaderLibraryName = "libvulkan.so";
#else
#error finish
#endif
    vulkanLibrary = std::shared_ptr<void>(SDL_LoadObject(vulkanLoaderLibraryName),
                                          [](void *p)
                                          {
                                              if(p)
                                                  SDL_UnloadObject(p);
                                          });
    if(!vulkanLibrary)
        throw std::runtime_error(std::string("can't load vulkan loader: ")
                                 + vulkanLoaderLibraryName);
    return std::make_shared<VulkanFunctions>(std::move(vulkanLibrary), PrivateAccess());
#endif
}
}
}
}
}
}
