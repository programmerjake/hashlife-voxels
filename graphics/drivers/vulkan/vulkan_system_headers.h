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

#ifndef GRAPHICS_DRIVERS_VULKAN_VULKAN_SYSTEM_HEADERS_H_
#define GRAPHICS_DRIVERS_VULKAN_VULKAN_SYSTEM_HEADERS_H_

#if 0
#define USE_PLATFORM_SDL_VULKAN
#if 1 && defined(__CDT_PARSER__)
#define SDL_WINDOW_VULKAN (static_cast<SDL_WindowFlags>(0x10000000))
typedef void *SDL_vulkanInstance; /* VK_DEFINE_HANDLE(VkInstance) */
typedef Uint64 SDL_vulkanSurface; /* VK_DEFINE_NON_DISPATCHABLE_HANDLE(VkSurfaceKHR) */
extern "C" int SDL_Vulkan_LoadLibrary(const char *path);
extern "C" void *SDL_Vulkan_GetVkGetInstanceProcAddr(void);
extern "C" void SDL_Vulkan_UnloadLibrary(void);
extern "C" SDL_bool SDL_Vulkan_GetInstanceExtensions(SDL_Window *window, unsigned *count, const char **names);
extern "C" SDL_bool SDL_Vulkan_CreateSurface(SDL_Window *window, SDL_vulkanInstance instance, SDL_vulkanSurface *surface);
#endif
#endif
#if defined(__ANDROID__) && !defined(USE_PLATFORM_SDL_VULKAN)
#error we need to modify SDL to not create an EGLSurface; enable Android after SDL is fixed
#define VK_USE_PLATFORM_ANDROID_KHR
#elif defined(__linux__)
#ifndef USE_PLATFORM_SDL_VULKAN
#define VK_USE_PLATFORM_XCB_KHR
#define VK_USE_PLATFORM_XLIB_KHR
#if 0
#error we need to modify SDL to not create an EGLSurface; enable Mir and Wayland after SDL is fixed
#define VK_USE_PLATFORM_MIR_KHR
#define VK_USE_PLATFORM_WAYLAND_KHR
#endif
#include <X11/Xlib-xcb.h>
// remove conflicting macros
#undef None
#endif
#elif defined(_WIN32)
#ifndef USE_PLATFORM_SDL_VULKAN
#define VK_USE_PLATFORM_WIN32_KHR
#endif
#define NOMINMAX
#include <windows.h>
#elif !defined(USE_PLATFORM_SDL_VULKAN)
#error unsupported platform
#endif
#define VK_NO_PROTOTYPES
#include <vulkan/vulkan.h>

#endif /* GRAPHICS_DRIVERS_VULKAN_VULKAN_SYSTEM_HEADERS_H_ */
