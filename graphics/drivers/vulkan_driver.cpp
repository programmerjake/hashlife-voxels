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
#include "vulkan_driver.h"
#if 1
#warning finish writing VulkanDriver
#else
#include <cassert>
#include <memory>
#include <stdexcept>
#include <vector>
#include <sstream>
#include "SDL_syswm.h"
#if defined(__ANDROID__)
#define VK_USE_PLATFORM_ANDROID_KHR
#elif defined(__linux__)
#define VK_USE_PLATFORM_XLIB_KHR
#else
#error unsupported platform
#endif
#define VK_NO_PROTOTYPES
#include <vulkan/vulkan.h>
#include <vulkan/vk_platform.h>

namespace programmerjake
{
namespace voxels
{
namespace graphics
{
namespace drivers
{
struct VulkanDriver::Implementation final
{
public:
    struct LibraryUnloader
    {
        void operator()(void *object) const
        {
            if(object)
                SDL_UnloadObject(object);
        }
    };
    struct WMHelper
    {
        virtual ~WMHelper() = default;
        SDL_SYSWM_TYPE syswmType;
        explicit WMHelper(SDL_SYSWM_TYPE syswmType) : syswmType(syswmType)
        {
        }
        virtual void addEnabledExtensions(std::vector<const char *> &extensions) const = 0;
    };

public:
    std::unique_ptr<void, LibraryUnloader> vulkanLoader = nullptr;
    PFN_vkGetInstanceProcAddr vkGetInstanceProcAddr = nullptr;
    PFN_vkGetDeviceProcAddr vkGetDeviceProcAddr = nullptr;
    VkInstance instance = nullptr;
    VkDevice device = nullptr;
    VulkanDriver *const vulkanDriver;
    const WMHelper *wmHelper = nullptr;

public:
#define VULKAN_DRIVER_FUNCTIONS()                                         \
    VULKAN_DRIVER_GLOBAL_FUNCTION(vkEnumerateInstanceLayerProperties)     \
    VULKAN_DRIVER_GLOBAL_FUNCTION(vkEnumerateInstanceExtensionProperties) \
    VULKAN_DRIVER_GLOBAL_FUNCTION(vkCreateInstance)                       \
    VULKAN_DRIVER_INSTANCE_FUNCTION(vkEnumeratePhysicalDevices)           \
    VULKAN_DRIVER_FUNCTIONS_END()
#define VULKAN_DRIVER_FUNCTIONS_END()
#define VULKAN_DRIVER_GLOBAL_FUNCTION(name) PFN_##name name = nullptr;
#define VULKAN_DRIVER_INSTANCE_FUNCTION(name) PFN_##name name = nullptr;
    VULKAN_DRIVER_FUNCTIONS()
#undef VULKAN_DRIVER_GLOBAL_FUNCTION
#undef VULKAN_DRIVER_INSTANCE_FUNCTION
public:
    void unloadFunctions()
    {
#define VULKAN_DRIVER_GLOBAL_FUNCTION(name) name = nullptr;
#define VULKAN_DRIVER_INSTANCE_FUNCTION(name) name = nullptr;
        VULKAN_DRIVER_FUNCTIONS()
#undef VULKAN_DRIVER_GLOBAL_FUNCTION
#undef VULKAN_DRIVER_INSTANCE_FUNCTION
    }
    template <typename Fn>
    void loadGlobalFunction(Fn &fn, const char *name)
    {
        fn = reinterpret_cast<Fn>(vkGetInstanceProcAddr(nullptr, name));
        if(!fn)
            throw std::runtime_error(std::string("can't load vulkan function: ") + name);
    }
    template <typename Fn>
    void loadInstanceFunction(Fn &fn, const char *name)
    {
        fn = reinterpret_cast<Fn>(vkGetInstanceProcAddr(instance, name));
        if(!fn)
            throw std::runtime_error(std::string("can't load vulkan function: ") + name);
    }
    void loadGlobalFunctions()
    {
        assert(instance == nullptr);
#define VULKAN_DRIVER_GLOBAL_FUNCTION(name) loadGlobalFunction(name, #name);
#define VULKAN_DRIVER_INSTANCE_FUNCTION(name)
        VULKAN_DRIVER_FUNCTIONS()
#undef VULKAN_DRIVER_GLOBAL_FUNCTION
#undef VULKAN_DRIVER_INSTANCE_FUNCTION
    }
    void loadInstanceFunctions()
    {
        assert(instance != nullptr);
#define VULKAN_DRIVER_GLOBAL_FUNCTION(name)
#define VULKAN_DRIVER_INSTANCE_FUNCTION(name) loadInstanceFunction(name, #name);
        VULKAN_DRIVER_FUNCTIONS()
#undef VULKAN_DRIVER_GLOBAL_FUNCTION
#undef VULKAN_DRIVER_INSTANCE_FUNCTION
    }
#undef VULKAN_DRIVER_FUNCTIONS_END
#undef VULKAN_DRIVER_FUNCTIONS
public:
    void loadLoader()
    {
        assert(!vulkanLoader);
#if defined(__linux__) && !defined(__ANDROID__) // non-android linux
        const char *vulkanLoaderLibraryName = "libvulkan.so.1";
#else
#error finish
#endif
        vulkanLoader.reset(SDL_LoadObject(vulkanLoaderLibraryName));
        if(!vulkanLoader)
            throw std::runtime_error(std::string("can't load vulkan loader: ")
                                     + vulkanLoaderLibraryName);
        vkGetInstanceProcAddr = reinterpret_cast<PFN_vkGetInstanceProcAddr>(
            SDL_LoadFunction(vulkanLoader.get(), "vkGetInstanceProcAddr"));
        if(!vkGetInstanceProcAddr)
            throw std::runtime_error("invalid vulkan loader: vkGetInstanceProcAddr not found");
    }
    explicit Implementation(VulkanDriver *vulkanDriver) : vulkanDriver(vulkanDriver)
    {
    }

public:
#ifdef VK_USE_PLATFORM_XLIB_KHR
    struct WMHelperXLib final : public WMHelper
    {
        WMHelperXLib() : WMHelper(SDL_SYSWM_TYPE::SDL_SYSWM_X11)
        {
        }
        virtual void addEnabledExtensions(std::vector<const char *> &extensions) const override
        {
            extensions.push_back(VK_KHR_XLIB_SURFACE_EXTENSION_NAME);
        }
    };
#endif
    class WMHelpers final
    {
    private:
        const WMHelper *const *front;
        const WMHelper *const *back;

    public:
        WMHelpers() noexcept
        {
#ifdef VK_USE_PLATFORM_XLIB_KHR
            static const WMHelperXLib helperXLib;
#endif
            static const WMHelper *const helpers[] = {
#ifdef VK_USE_PLATFORM_XLIB_KHR
                &helperXLib,
#endif
            };
            front = &helpers[0];
            back = &helpers[sizeof(helpers) / sizeof(helpers[0])];
        }
        std::size_t size() const
        {
            return back - front;
        }
        const WMHelper *const *begin() const
        {
            return front;
        }
        const WMHelper *const *end() const
        {
            return back;
        }
    };

public:
    void createGraphicsContext()
    {
        assert(!wmHelper);
        SDL_SysWMinfo wmInfo{};
        SDL_VERSION(&wmInfo.version);
        if(!SDL_GetWindowWMInfo(vulkanDriver->getWindow(), &wmInfo))
            throw std::runtime_error(std::string("SDL_GetWindowWMInfo failed: ") + SDL_GetError());
        wmHelper = nullptr;
        for(auto helper : WMHelpers())
        {
            if(helper->syswmType == wmInfo.subsystem)
            {
                wmHelper = helper;
                break;
            }
        }
        if(!wmHelper)
        {
            switch(wmInfo.subsystem)
            {
            case SDL_SYSWM_WINDOWS:
                throw std::runtime_error("SDL_SYSWM_WINDOWS is not supported");
            case SDL_SYSWM_X11:
                throw std::runtime_error("SDL_SYSWM_X11 is not supported");
            case SDL_SYSWM_DIRECTFB:
                throw std::runtime_error("SDL_SYSWM_DIRECTFB is not supported");
            case SDL_SYSWM_COCOA:
                throw std::runtime_error("SDL_SYSWM_COCOA is not supported");
            case SDL_SYSWM_UIKIT:
                throw std::runtime_error("SDL_SYSWM_UIKIT is not supported");
            case SDL_SYSWM_WAYLAND:
                throw std::runtime_error("SDL_SYSWM_WAYLAND is not supported");
            case SDL_SYSWM_MIR:
                throw std::runtime_error("SDL_SYSWM_MIR is not supported");
            case SDL_SYSWM_WINRT:
                throw std::runtime_error("SDL_SYSWM_WINRT is not supported");
            case SDL_SYSWM_ANDROID:
                throw std::runtime_error("SDL_SYSWM_ANDROID is not supported");
            default:
                throw std::runtime_error("unsupported SDL_SYSWM_TYPE value");
            }
        }
        loadGlobalFunctions();
        VkApplicationInfo appInfo = {};
        appInfo.sType == VK_STRUCTURE_TYPE_APPLICATION_INFO;
        appInfo.apiVersion = VK_API_VERSION_1_0;
#warning give vulkan our application name and version
        std::vector<const char *> enabledExtensions;
        enabledExtensions.push_back(VK_KHR_SURFACE_EXTENSION_NAME);
        wmHelper->addEnabledExtensions(enabledExtensions);
        VkInstanceCreateInfo instanceCreateInfo = {};
        instanceCreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        handleVulkanResult(vkCreateInstance(&instanceCreateInfo, nullptr, &instance),
                           "vkCreateInstance");
        loadInstanceFunctions();
    }
    void destroyGraphicsContext()
    {
#error finish
    }
    VkResult handleVulkanResult(VkResult result, const char *functionName)
    {
        if(result >= 0)
            return;
        constexpr auto VK_ERROR_FRAGMENTED_POOL = -12; // not all vulkan.h files have this
        switch(result)
        {
        case VK_ERROR_OUT_OF_HOST_MEMORY:
            throw std::runtime_error(std::string("vulkan error: ") + functionName
                                     + " returned VK_ERROR_OUT_OF_HOST_MEMORY");
        case VK_ERROR_OUT_OF_DEVICE_MEMORY:
            throw std::runtime_error(std::string("vulkan error: ") + functionName
                                     + " returned VK_ERROR_OUT_OF_DEVICE_MEMORY");
        case VK_ERROR_INITIALIZATION_FAILED:
            throw std::runtime_error(std::string("vulkan error: ") + functionName
                                     + " returned VK_ERROR_INITIALIZATION_FAILED");
        case VK_ERROR_DEVICE_LOST:
            throw std::runtime_error(std::string("vulkan error: ") + functionName
                                     + " returned VK_ERROR_DEVICE_LOST");
        case VK_ERROR_MEMORY_MAP_FAILED:
            throw std::runtime_error(std::string("vulkan error: ") + functionName
                                     + " returned VK_ERROR_MEMORY_MAP_FAILED");
        case VK_ERROR_LAYER_NOT_PRESENT:
            throw std::runtime_error(std::string("vulkan error: ") + functionName
                                     + " returned VK_ERROR_LAYER_NOT_PRESENT");
        case VK_ERROR_EXTENSION_NOT_PRESENT:
            throw std::runtime_error(std::string("vulkan error: ") + functionName
                                     + " returned VK_ERROR_EXTENSION_NOT_PRESENT");
        case VK_ERROR_FEATURE_NOT_PRESENT:
            throw std::runtime_error(std::string("vulkan error: ") + functionName
                                     + " returned VK_ERROR_FEATURE_NOT_PRESENT");
        case VK_ERROR_INCOMPATIBLE_DRIVER:
            throw std::runtime_error(std::string("vulkan error: ") + functionName
                                     + " returned VK_ERROR_INCOMPATIBLE_DRIVER");
        case VK_ERROR_TOO_MANY_OBJECTS:
            throw std::runtime_error(std::string("vulkan error: ") + functionName
                                     + " returned VK_ERROR_TOO_MANY_OBJECTS");
        case VK_ERROR_FORMAT_NOT_SUPPORTED:
            throw std::runtime_error(std::string("vulkan error: ") + functionName
                                     + " returned VK_ERROR_FORMAT_NOT_SUPPORTED");
        case VK_ERROR_FRAGMENTED_POOL:
            throw std::runtime_error(std::string("vulkan error: ") + functionName
                                     + " returned VK_ERROR_FRAGMENTED_POOL");
        default:
        {
            std::ostringstream ss;
            ss << "vulkan error: " << functionName << " returned " << result;
            throw std::runtime_error(ss.str());
        }
        }
    }
};

void VulkanDriver::renderFrame(std::shared_ptr<CommandBuffer> commandBuffer)
{
#error finish
}

SDL_Window *VulkanDriver::createWindow(int x, int y, int w, int h, std::uint32_t flags)
{
    if(!implementation->vulkanLoader)
        implementation->loadLoader();
    auto window = SDL_CreateWindow(getTitle().c_str(), x, y, w, h, flags);
    if(window)
        return window;
    throw std::runtime_error(std::string("SDL_CreateWindow failed: ") + SDL_GetError());
}

void VulkanDriver::createGraphicsContext()
{
    implementation->createGraphicsContext();
}

void VulkanDriver::destroyGraphicsContext() noexcept
{
    implementation->destroyGraphicsContext();
}

TextureId VulkanDriver::makeTexture(const std::shared_ptr<const Image> &image)
{
#error finish
}

void VulkanDriver::setNewImageData(TextureId texture, const std::shared_ptr<const Image> &image)
{
#error finish
}

std::shared_ptr<RenderBuffer> VulkanDriver::makeBuffer(
    const util::EnumArray<std::size_t, RenderLayer> &maximumSizes)
{
#error finish
}

std::shared_ptr<CommandBuffer> VulkanDriver::makeCommandBuffer()
{
#error finish
}

void VulkanDriver::setGraphicsContextRecreationNeeded() noexcept
{
#error finish
}
}
}
}
}
#endif
