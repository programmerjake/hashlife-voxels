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
#include "wmhelper.h"
#include "vulkan_object.h"
#ifndef USE_PLATFORM_SDL_VULKAN
#include "SDL_syswm.h"
#endif
#include <type_traits>
#include <string>

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
namespace
{
struct SurfaceHolder final
{
    SurfaceHolder(const SurfaceHolder &) = delete;
    SurfaceHolder &operator=(const SurfaceHolder &) = delete;
    const std::shared_ptr<const VulkanInstance> vulkanInstance;
    const std::shared_ptr<const VulkanFunctions> vk;
    VkSurfaceKHR surface = VK_NULL_HANDLE;
    bool valid = false;
    explicit SurfaceHolder(std::shared_ptr<const VulkanInstance> vulkanInstance)
        : vulkanInstance(std::move(vulkanInstance)), vk(this->vulkanInstance->vk)
    {
    }
    ~SurfaceHolder()
    {
        if(valid)
            vk->DestroySurfaceKHR(vulkanInstance->instance, surface, nullptr);
    }
};

#ifdef USE_PLATFORM_SDL_VULKAN
struct WMHelperSDLVulkan final : public WMHelper
{
    virtual bool isUsableWithWindow(SDL_Window *window) const override
    {
        return SDL_GetWindowFlags(window) & SDL_WINDOW_VULKAN;
    }
    virtual std::vector<const char *> getWMExtensionNames(SDL_Window *window) const override
    {
        std::vector<const char *> names;
        unsigned extensionCount;
        if(!SDL_Vulkan_GetInstanceExtensions(window, &extensionCount, nullptr))
            throw std::runtime_error(std::string("SDL_Vulkan_GetInstanceExtensions failed: ")
                                     + SDL_GetError());
        names.resize(extensionCount);
        if(!SDL_Vulkan_GetInstanceExtensions(window, &extensionCount, names.data()))
            throw std::runtime_error(std::string("SDL_Vulkan_GetInstanceExtensions failed: ")
                                     + SDL_GetError());
        std::string surfaceExtensionName = VK_KHR_SURFACE_EXTENSION_NAME;
        for(auto i = names.begin(); i != names.end(); ++i)
        {
            if(surfaceExtensionName == *i)
            {
                names.erase(i); // erase surface extension name here so we don't request it twice
                break;
            }
        }
        return names;
    }
    virtual std::shared_ptr<const VkSurfaceKHR> createSurface(
        std::shared_ptr<const VulkanInstance> vulkanInstanceIn, SDL_Window *window) const override
    {
        auto holder = std::make_shared<SurfaceHolder>(vulkanInstanceIn);
        if(!SDL_Vulkan_CreateSurface(
               window,
               static_cast<SDL_vulkanInstance>(holder->vulkanInstance->instance),
               reinterpret_cast<SDL_vulkanSurface *>(&holder->surface)))
            throw std::runtime_error(std::string("SDL_Vulkan_CreateSurface failed: ")
                                     + SDL_GetError());
        holder->valid = true;
        return std::shared_ptr<const VkSurfaceKHR>(holder, &holder->surface);
    }
};
#else
void getWindowWMInfo(SDL_Window *window, SDL_SysWMinfo &wmInfo)
{
    wmInfo = {};
    SDL_VERSION(&wmInfo.version);
    if(!SDL_GetWindowWMInfo(window, &wmInfo))
        throw std::runtime_error(std::string("SDL_GetWindowWMInfo failed: ") + SDL_GetError());
}
#endif

#ifdef VK_USE_PLATFORM_XLIB_KHR
struct WMHelperXLib final : public WMHelper
{
    virtual bool isUsableWithWindow(SDL_Window *window) const override
    {
        SDL_SysWMinfo wmInfo;
        getWindowWMInfo(window, wmInfo);
        return wmInfo.subsystem == SDL_SYSWM_TYPE::SDL_SYSWM_X11;
    }
    virtual std::vector<const char *> getWMExtensionNames(SDL_Window *window) const override
    {
        return {VK_KHR_XLIB_SURFACE_EXTENSION_NAME};
    }
    virtual std::shared_ptr<const VkSurfaceKHR> createSurface(
        std::shared_ptr<const VulkanInstance> vulkanInstanceIn, SDL_Window *window) const override
    {
        SDL_SysWMinfo wmInfo;
        getWindowWMInfo(window, wmInfo);
        auto holder = std::make_shared<SurfaceHolder>(vulkanInstanceIn);
        PFN_vkCreateXlibSurfaceKHR vkCreateXlibSurfaceKHR = nullptr;
        holder->vk->loadInstanceFunction(
            vkCreateXlibSurfaceKHR, holder->vulkanInstance->instance, "vkCreateXlibSurfaceKHR");
        VkXlibSurfaceCreateInfoKHR createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_XLIB_SURFACE_CREATE_INFO_KHR;
        createInfo.dpy = wmInfo.info.x11.display;
        createInfo.window = wmInfo.info.x11.window;
        handleVulkanResult(
            vkCreateXlibSurfaceKHR(
                holder->vulkanInstance->instance, &createInfo, nullptr, &holder->surface),
            "vkCreateXlibSurfaceKHR");
        holder->valid = true;
        return std::shared_ptr<const VkSurfaceKHR>(holder, &holder->surface);
    }
};
#endif
#ifdef VK_USE_PLATFORM_XCB_KHR
struct WMHelperXCB final : public WMHelper
{
    virtual bool isUsableWithWindow(SDL_Window *window) const override
    {
        SDL_SysWMinfo wmInfo;
        getWindowWMInfo(window, wmInfo);
        return wmInfo.subsystem == SDL_SYSWM_TYPE::SDL_SYSWM_X11;
    }
    virtual std::vector<const char *> getWMExtensionNames(SDL_Window *window) const override
    {
        return {VK_KHR_XCB_SURFACE_EXTENSION_NAME};
    }
    virtual std::shared_ptr<const VkSurfaceKHR> createSurface(
        std::shared_ptr<const VulkanInstance> vulkanInstanceIn, SDL_Window *window) const override
    {
        SDL_SysWMinfo wmInfo;
        getWindowWMInfo(window, wmInfo);
        struct Holder final
        {
            SurfaceHolder surfaceHolder;
            std::shared_ptr<void> xlibXCBLibrary;
            explicit Holder(std::shared_ptr<const VulkanInstance> vulkanInstance)
                : surfaceHolder(std::move(vulkanInstance))
            {
            }
        };
        auto holder = std::make_shared<Holder>(vulkanInstanceIn);
        const auto xlibXCBLibraryName = "libX11-xcb.so.1";
        holder->xlibXCBLibrary = std::shared_ptr<void>(SDL_LoadObject(xlibXCBLibraryName),
                                                       [](void *p)
                                                       {
                                                           SDL_UnloadObject(p);
                                                       });
        if(!holder->xlibXCBLibrary)
            throw std::runtime_error(std::string("can't load library ") + xlibXCBLibraryName);
        decltype(XGetXCBConnection) *getXCBConnection;
        getXCBConnection = reinterpret_cast<decltype(getXCBConnection)>(
            SDL_LoadFunction(holder->xlibXCBLibrary.get(), "XGetXCBConnection"));
        if(!getXCBConnection)
            throw std::runtime_error("can't load function XGetXCBConnection");
        PFN_vkCreateXcbSurfaceKHR vkCreateXcbSurfaceKHR = nullptr;
        holder->surfaceHolder.vk->loadInstanceFunction(
            vkCreateXcbSurfaceKHR,
            holder->surfaceHolder.vulkanInstance->instance,
            "vkCreateXcbSurfaceKHR");
        VkXcbSurfaceCreateInfoKHR createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_XCB_SURFACE_CREATE_INFO_KHR;
        createInfo.connection = getXCBConnection(wmInfo.info.x11.display);
        createInfo.window = static_cast<xcb_window_t>(wmInfo.info.x11.window);
        handleVulkanResult(vkCreateXcbSurfaceKHR(holder->surfaceHolder.vulkanInstance->instance,
                                                 &createInfo,
                                                 nullptr,
                                                 &holder->surfaceHolder.surface),
                           "vkCreateXcbSurfaceKHR");
        holder->surfaceHolder.valid = true;
        return std::shared_ptr<const VkSurfaceKHR>(holder, &holder->surfaceHolder.surface);
    }
};
#endif
#ifdef VK_USE_PLATFORM_WAYLAND_KHR
#error we need to modify SDL to not create an EGLSurface
struct WMHelperWayland final : public WMHelper
{
    virtual bool isUsableWithWindow(SDL_Window *window) const override
    {
        SDL_SysWMinfo wmInfo;
        getWindowWMInfo(window, wmInfo);
        return wmInfo.subsystem == SDL_SYSWM_TYPE::SDL_SYSWM_WAYLAND;
    }
    virtual std::vector<const char *> getWMExtensionNames(SDL_Window *window) const override
    {
        return {VK_KHR_WAYLAND_SURFACE_EXTENSION_NAME};
    }
    virtual std::shared_ptr<const VkSurfaceKHR> createSurface(
        std::shared_ptr<const VulkanInstance> vulkanInstanceIn, SDL_Window *window) const override
    {
        SDL_SysWMinfo wmInfo;
        getWindowWMInfo(window, wmInfo);
        auto holder = std::make_shared<SurfaceHolder>(vulkanInstanceIn);
        PFN_vkCreateWaylandSurfaceKHR vkCreateWaylandSurfaceKHR = nullptr;
        holder->vk->loadInstanceFunction(vkCreateWaylandSurfaceKHR,
                                         holder->vulkanInstance->instance,
                                         "vkCreateWaylandSurfaceKHR");
        VkWaylandSurfaceCreateInfoKHR createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_WAYLAND_SURFACE_CREATE_INFO_KHR;
        createInfo.display = wmInfo.info.wl.display;
        createInfo.surface = wmInfo.info.wl.surface;
        handleVulkanResult(
            vkCreateWaylandSurfaceKHR(
                holder->vulkanInstance->instance, &createInfo, nullptr, &holder->surface),
            "vkCreateWaylandSurfaceKHR");
        holder->valid = true;
        return std::shared_ptr<const VkSurfaceKHR>(holder, &holder->surface);
    }
};
#endif
#ifdef VK_USE_PLATFORM_MIR_KHR
#error we need to modify SDL to not create an EGLSurface
struct WMHelperMir final : public WMHelper
{
    virtual bool isUsableWithWindow(SDL_Window *window) const override
    {
        SDL_SysWMinfo wmInfo;
        getWindowWMInfo(window, wmInfo);
        return wmInfo.subsystem == SDL_SYSWM_TYPE::SDL_SYSWM_MIR;
    }
    virtual std::vector<const char *> getWMExtensionNames(SDL_Window *window) const override
    {
        return {VK_KHR_MIR_SURFACE_EXTENSION_NAME};
    }
    virtual std::shared_ptr<const VkSurfaceKHR> createSurface(
        std::shared_ptr<const VulkanInstance> vulkanInstanceIn, SDL_Window *window) const override
    {
        SDL_SysWMinfo wmInfo;
        getWindowWMInfo(window, wmInfo);
        auto holder = std::make_shared<SurfaceHolder>(vulkanInstanceIn);
        PFN_vkCreateMirSurfaceKHR vkCreateMirSurfaceKHR = nullptr;
        holder->vk->loadInstanceFunction(
            vkCreateMirSurfaceKHR, holder->vulkanInstance->instance, "vkCreateMirSurfaceKHR");
        VkMirSurfaceCreateInfoKHR createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_MIR_SURFACE_CREATE_INFO_KHR;
        createInfo.connection = wmInfo.info.mir.connection;
        createInfo.mirSurface = wmInfo.info.mir.surface;
        handleVulkanResult(
            vkCreateMirSurfaceKHR(
                holder->vulkanInstance->instance, &createInfo, nullptr, &holder->surface),
            "vkCreateMirSurfaceKHR");
        holder->valid = true;
        return std::shared_ptr<const VkSurfaceKHR>(holder, &holder->surface);
    }
};
#endif
#ifdef VK_USE_PLATFORM_ANDROID_KHR
#error finish
#error we need to modify SDL to not create an EGLSurface
struct WMHelperAndroid final : public WMHelper
{
    virtual bool isUsableWithWindow(SDL_Window *window) const override
    {
        SDL_SysWMinfo wmInfo;
        getWindowWMInfo(window, wmInfo);
        return wmInfo.subsystem == SDL_SYSWM_TYPE::SDL_SYSWM_ANDROID;
    }
    virtual std::vector<const char *> getWMExtensionNames(SDL_Window *window) const override
    {
        return {VK_KHR_MIR_SURFACE_EXTENSION_NAME};
    }
    virtual std::shared_ptr<const VkSurfaceKHR> createSurface(
        std::shared_ptr<const VulkanInstance> vulkanInstanceIn, SDL_Window *window) const override
    {
        SDL_SysWMinfo wmInfo;
        getWindowWMInfo(window, wmInfo);
        auto holder = std::make_shared<SurfaceHolder>(vulkanInstanceIn);
        PFN_vkCreateMirSurfaceKHR vkCreateMirSurfaceKHR = nullptr;
        holder->vk->loadInstanceFunction(
            vkCreateMirSurfaceKHR, holder->vulkanInstance->instance, "vkCreateMirSurfaceKHR");
        VkMirSurfaceCreateInfoKHR createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_MIR_SURFACE_CREATE_INFO_KHR;
        createInfo.connection = wmInfo.info.mir.connection;
        createInfo.mirSurface = wmInfo.info.mir.surface;
        handleVulkanResult(
            vkCreateMirSurfaceKHR(
                holder->vulkanInstance->instance, &createInfo, nullptr, &holder->surface),
            "vkCreateMirSurfaceKHR");
        holder->valid = true;
        return std::shared_ptr<const VkSurfaceKHR>(holder, &holder->surface);
    }
};
#endif
#ifdef VK_USE_PLATFORM_WIN32_KHR
struct WMHelperWin32 final : public WMHelper
{
    virtual bool isUsableWithWindow(SDL_Window *window) const override
    {
        SDL_SysWMinfo wmInfo;
        getWindowWMInfo(window, wmInfo);
        return wmInfo.subsystem == SDL_SYSWM_TYPE::SDL_SYSWM_WINDOWS;
    }
    virtual std::vector<const char *> getWMExtensionNames(SDL_Window *window) const override
    {
        return {VK_KHR_WIN32_SURFACE_EXTENSION_NAME};
    }
    virtual std::shared_ptr<const VkSurfaceKHR> createSurface(
        std::shared_ptr<const VulkanInstance> vulkanInstanceIn, SDL_Window *window) const override
    {
        SDL_SysWMinfo wmInfo;
        getWindowWMInfo(window, wmInfo);
        auto holder = std::make_shared<SurfaceHolder>(vulkanInstanceIn);
        PFN_vkCreateWin32SurfaceKHR vkCreateWin32SurfaceKHR = nullptr;
        holder->vk->loadInstanceFunction(
            vkCreateWin32SurfaceKHR, holder->vulkanInstance->instance, "vkCreateWin32SurfaceKHR");
        VkWin32SurfaceCreateInfoKHR createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
        createInfo.hinstance = ::GetWindowLongPtr(wmInfo.info.win.window, GWLP_HINSTANCE);
        createInfo.hwnd = wmInfo.info.win.window;
        handleVulkanResult(
            vkCreateWin32SurfaceKHR(
                holder->vulkanInstance->instance, &createInfo, nullptr, &holder->surface),
            "vkCreateXcbSurfaceKHR");
        holder->valid = true;
        return std::shared_ptr<const VkSurfaceKHR>(holder, &holder->surface);
    }
};
#endif
}
WMHelpers::WMHelpers() noexcept
{
#ifdef USE_PLATFORM_SDL_VULKAN
    static const WMHelperSDLVulkan helperSDLVulkan;
#endif
#ifdef VK_USE_PLATFORM_XLIB_KHR
    static const WMHelperXLib helperXLib;
#endif
#ifdef VK_USE_PLATFORM_XCB_KHR
    static const WMHelperXCB helperXCB;
#endif
#ifdef VK_USE_PLATFORM_WAYLAND_KHR
    static const WMHelperWayland helperWayland;
#endif
#ifdef VK_USE_PLATFORM_MIR_KHR
    static const WMHelperMir helperMir;
#endif
#ifdef VK_USE_PLATFORM_ANDROID_KHR
    static const WMHelperAndroid helperAndroid;
#endif
#ifdef VK_USE_PLATFORM_WIN32_KHR
    static const WMHelperWin32 helperWin32;
#endif
    static const WMHelper *const helpers[] = {
#ifdef USE_PLATFORM_SDL_VULKAN
        &helperSDLVulkan,
#endif
#ifdef VK_USE_PLATFORM_WAYLAND_KHR
        &helperWayland,
#endif
#ifdef VK_USE_PLATFORM_MIR_KHR
        &helperMir,
#endif
#ifdef VK_USE_PLATFORM_XLIB_KHR
        &helperXLib, // XLib before XCB so we don't require XLib XCB interop to use Vulkan
#endif
#ifdef VK_USE_PLATFORM_XCB_KHR
        &helperXCB,
#endif
#ifdef VK_USE_PLATFORM_ANDROID_KHR
        &helperAndroid,
#endif
#ifdef VK_USE_PLATFORM_WIN32_KHR
        &helperWin32,
#endif
    };
    front = &helpers[0];
    back = front + sizeof(helpers) / sizeof(helpers[0]);
}
}
}
}
}
}
