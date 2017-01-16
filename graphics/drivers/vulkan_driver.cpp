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
#if 0
#warning finish writing VulkanDriver
#else
#include <cassert>
#include <memory>
#include <stdexcept>
#include <vector>
#include <sstream>
#include <cstdint>
#include <utility>
#include <type_traits>
#include "SDL_syswm.h"
#if defined(__ANDROID__)
#define VK_USE_PLATFORM_ANDROID_KHR
#elif defined(__linux__)
#define VK_USE_PLATFORM_XCB_KHR
#include <X11/Xlib-xcb.h>
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
    struct WMHelper
    {
        virtual ~WMHelper() = default;
        SDL_SYSWM_TYPE syswmType;
        explicit WMHelper(SDL_SYSWM_TYPE syswmType) : syswmType(syswmType)
        {
        }
        virtual void addEnabledInstanceExtensions(std::vector<const char *> &extensions) const = 0;
    };
    struct InstanceHolder final
    {
        std::shared_ptr<void> vulkanLoader;
        PFN_vkDestroyInstance vkDestroyInstance = nullptr;
        VkInstance instance = nullptr;
        explicit InstanceHolder(std::shared_ptr<void> vulkanLoader)
            : vulkanLoader(std::move(vulkanLoader))
        {
        }
        void loadDestroyFunction(Implementation *implementation)
        {
            assert(instance);
            implementation->loadInstanceFunction(vkDestroyInstance, instance, "vkDestroyInstance");
        }
        ~InstanceHolder()
        {
            if(!vkDestroyInstance) // instance could have been set to garbage by a failing
                // vkCreateInstance; check vkDestroyInstance instead
                return;
            vkDestroyInstance(instance, nullptr);
        }
        InstanceHolder(InstanceHolder &&rt)
            : vulkanLoader(std::move(rt.vulkanLoader)),
              vkDestroyInstance(rt.vkDestroyInstance),
              instance(rt.instance)
        {
            rt.vkDestroyInstance = nullptr;
            rt.instance = nullptr;
        }
        InstanceHolder &operator=(InstanceHolder rt)
        {
            vulkanLoader.swap(rt.vulkanLoader);
            std::swap(vkDestroyInstance, rt.vkDestroyInstance);
            std::swap(instance, rt.instance);
            return *this;
        }
    };
    struct DeviceHolder final
    {
        std::shared_ptr<const VkInstance> instance;
        PFN_vkDestroyDevice vkDestroyDevice = nullptr;
        VkDevice device = nullptr;
        explicit DeviceHolder(std::shared_ptr<const VkInstance> instance)
            : instance(std::move(instance))
        {
        }
        void loadDestroyFunction(Implementation *implementation)
        {
            assert(instance);
            implementation->loadDeviceFunction(vkDestroyDevice, device, "vkDestroyDevice");
        }
        ~DeviceHolder()
        {
            if(!vkDestroyDevice) // device could have been set to garbage by a failing
                // vkCreateDevice; check vkDestroyDevice instead
                return;
            vkDestroyDevice(device, nullptr);
        }
        DeviceHolder(DeviceHolder &&rt)
            : instance(std::move(rt.instance)),
              vkDestroyDevice(rt.vkDestroyDevice),
              device(rt.device)
        {
            rt.vkDestroyDevice = nullptr;
            rt.device = nullptr;
        }
        DeviceHolder &operator=(DeviceHolder rt)
        {
            instance.swap(rt.instance);
            std::swap(vkDestroyDevice, rt.vkDestroyDevice);
            std::swap(device, rt.device);
            return *this;
        }
    };

public:
    VulkanDriver *const vulkanDriver;
    std::shared_ptr<void> vulkanLoader = nullptr;
    PFN_vkGetInstanceProcAddr vkGetInstanceProcAddr = nullptr;
    std::shared_ptr<const VkInstance> instance = nullptr;
    const WMHelper *wmHelper = nullptr;
    VkPhysicalDeviceProperties physicalDeviceProperties;
    VkPhysicalDeviceFeatures physicalDeviceFeatures;
    std::uint32_t queueFamilyIndex = 0;
    std::shared_ptr<const VkPhysicalDevice> physicalDevice = nullptr;
    std::shared_ptr<const VkDevice> device = nullptr;

public:
#define VULKAN_DRIVER_GLOBAL_FUNCTION(name) PFN_##name name = nullptr;
#define VULKAN_DRIVER_INSTANCE_FUNCTION(name) PFN_##name name = nullptr;
#define VULKAN_DRIVER_DEVICE_FUNCTION(name) PFN_##name name = nullptr;
#include "vulkan_driver_functions.h"
public:
    void unloadFunctions()
    {
#define VULKAN_DRIVER_GLOBAL_FUNCTION(name) name = nullptr;
#define VULKAN_DRIVER_INSTANCE_FUNCTION(name) name = nullptr;
#define VULKAN_DRIVER_DEVICE_FUNCTION(name) name = nullptr;
#include "vulkan_driver_functions.h"
    }
    template <typename Fn>
    void loadGlobalFunction(Fn &fn, const char *name) const
    {
        fn = reinterpret_cast<Fn>(vkGetInstanceProcAddr(nullptr, name));
        if(!fn)
            throw std::runtime_error(std::string("can't load vulkan function: ") + name);
    }
    template <typename Fn>
    void loadInstanceFunction(Fn &fn, VkInstance instance2, const char *name) const
    {
        fn = reinterpret_cast<Fn>(vkGetInstanceProcAddr(instance2, name));
        if(!fn)
            throw std::runtime_error(std::string("can't load vulkan function: ") + name);
    }
    template <typename Fn>
    void loadInstanceFunction(Fn &fn, const char *name) const
    {
        loadInstanceFunction(fn, *instance, name);
    }
    template <typename Fn>
    void loadDeviceFunction(Fn &fn, VkDevice device2, const char *name) const
    {
        fn = reinterpret_cast<Fn>(vkGetDeviceProcAddr(device2, name));
        if(!fn)
            throw std::runtime_error(std::string("can't load vulkan function: ") + name);
    }
    template <typename Fn>
    void loadDeviceFunction(Fn &fn, const char *name) const
    {
        loadDeviceFunction(fn, *device, name);
    }
    void loadGlobalFunctions()
    {
        assert(instance == nullptr);
#define VULKAN_DRIVER_GLOBAL_FUNCTION(name) loadGlobalFunction(name, #name);
#define VULKAN_DRIVER_INSTANCE_FUNCTION(name)
#define VULKAN_DRIVER_DEVICE_FUNCTION(name)
#include "vulkan_driver_functions.h"
    }
    void loadInstanceFunctions()
    {
        assert(instance != nullptr);
#define VULKAN_DRIVER_GLOBAL_FUNCTION(name)
#define VULKAN_DRIVER_INSTANCE_FUNCTION(name) loadInstanceFunction(name, #name);
#define VULKAN_DRIVER_DEVICE_FUNCTION(name)
#include "vulkan_driver_functions.h"
    }
    void loadDeviceFunctions()
    {
        assert(instance != nullptr);
#define VULKAN_DRIVER_GLOBAL_FUNCTION(name)
#define VULKAN_DRIVER_INSTANCE_FUNCTION(name) loadInstanceFunction(name, #name);
#define VULKAN_DRIVER_DEVICE_FUNCTION(name) loadDeviceFunction(name, #name);
#include "vulkan_driver_functions.h"
    }

public:
    void loadLoader()
    {
        assert(!vulkanLoader);
#if defined(__linux__) && !defined(__ANDROID__) // non-android linux
        const char *vulkanLoaderLibraryName = "libvulkan.so.1";
#else
#error finish
#endif
        vulkanLoader = std::shared_ptr<void>(SDL_LoadObject(vulkanLoaderLibraryName),
                                             [](void *p)
                                             {
                                                 if(p)
                                                     SDL_UnloadObject(p);
                                             });
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
#ifdef VK_USE_PLATFORM_XCB_KHR
    struct WMHelperXCB final : public WMHelper
    {
        WMHelperXCB() : WMHelper(SDL_SYSWM_TYPE::SDL_SYSWM_X11)
        {
        }
        virtual void addEnabledInstanceExtensions(
            std::vector<const char *> &extensions) const override
        {
            extensions.push_back(VK_KHR_XCB_SURFACE_EXTENSION_NAME);
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
#ifdef VK_USE_PLATFORM_XCB_KHR
            static const WMHelperXCB helperXCB;
#endif
            static const WMHelper *const helpers[] = {
#ifdef VK_USE_PLATFORM_XCB_KHR
                &helperXCB,
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
        appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        appInfo.apiVersion = VK_API_VERSION_1_0;
#warning give vulkan our application name and version
        VkInstanceCreateInfo instanceCreateInfo = {};
        instanceCreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        instanceCreateInfo.pApplicationInfo = &appInfo;
        std::vector<const char *> enabledInstanceLayers;
        instanceCreateInfo.enabledLayerCount = enabledInstanceLayers.size();
        instanceCreateInfo.ppEnabledLayerNames = enabledInstanceLayers.data();
        std::vector<const char *> enabledInstanceExtensions;
        enabledInstanceExtensions.push_back(VK_KHR_SURFACE_EXTENSION_NAME);
        wmHelper->addEnabledInstanceExtensions(enabledInstanceExtensions);
        instanceCreateInfo.enabledExtensionCount = enabledInstanceExtensions.size();
        instanceCreateInfo.ppEnabledExtensionNames = enabledInstanceExtensions.data();
        {
            auto instanceHolder = std::make_shared<InstanceHolder>(vulkanLoader);
            handleVulkanResult(
                vkCreateInstance(&instanceCreateInfo, nullptr, &instanceHolder->instance),
                "vkCreateInstance");
            instanceHolder->loadDestroyFunction(this);
            instance = std::shared_ptr<const VkInstance>(instanceHolder, &instanceHolder->instance);
        }
        loadInstanceFunctions();
        std::uint32_t physicalDeviceCount = 0;
        handleVulkanResult(vkEnumeratePhysicalDevices(*instance, &physicalDeviceCount, nullptr),
                           "vkEnumeratePhysicalDevices");
        if(physicalDeviceCount == 0)
            throw std::runtime_error("Vulkan: no physical devices");
        std::vector<VkPhysicalDevice> physicalDevices;
        physicalDevices.resize(physicalDeviceCount);
        handleVulkanResult(
            vkEnumeratePhysicalDevices(*instance, &physicalDeviceCount, physicalDevices.data()),
            "vkEnumeratePhysicalDevices");
        assert(physicalDevices.size() == physicalDeviceCount);
        physicalDevice = nullptr;
        std::vector<VkQueueFamilyProperties> queueFamiliesProperties;
        std::vector<VkExtensionProperties> deviceExtensions;
        const std::string swapchainExtensionName = VK_KHR_SWAPCHAIN_EXTENSION_NAME;
        for(auto i : physicalDevices)
        {
            vkGetPhysicalDeviceProperties(i, &physicalDeviceProperties);
            if(VK_VERSION_MAJOR(physicalDeviceProperties.apiVersion) < 1)
                continue;
            vkGetPhysicalDeviceFeatures(i, &physicalDeviceFeatures);
            std::uint32_t queueFamiliesCount = 0;
            vkGetPhysicalDeviceQueueFamilyProperties(i, &queueFamiliesCount, nullptr);
            if(queueFamiliesCount == 0)
                continue;
            queueFamiliesProperties.resize(queueFamiliesCount);
            vkGetPhysicalDeviceQueueFamilyProperties(
                i, &queueFamiliesCount, queueFamiliesProperties.data());
            assert(queueFamiliesProperties.size() == queueFamiliesCount);
            queueFamilyIndex = queueFamiliesCount;
            for(std::uint32_t j = 0; j < queueFamiliesCount; j++)
            {
                if((queueFamiliesProperties[j].queueFlags & VK_QUEUE_GRAPHICS_BIT) == 0)
                    continue;
                if(queueFamiliesProperties[j].queueCount == 0)
                    continue;
                queueFamilyIndex = j;
                break;
            }
            if(queueFamilyIndex == queueFamiliesCount) // no good queues found
                continue;
            std::uint32_t deviceExtensionCount = 0;
            handleVulkanResult(
                vkEnumerateDeviceExtensionProperties(i, nullptr, &deviceExtensionCount, nullptr),
                "vkEnumerateDeviceExtensionProperties");
            deviceExtensions.resize(deviceExtensionCount);
            if(deviceExtensionCount > 0)
                handleVulkanResult(vkEnumerateDeviceExtensionProperties(
                                       i, nullptr, &deviceExtensionCount, deviceExtensions.data()),
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
            struct PhysicalDeviceAndInstance final
            {
                std::shared_ptr<const VkInstance> instance;
                const VkPhysicalDevice physicalDevice;
                PhysicalDeviceAndInstance(std::shared_ptr<const VkInstance> instance,
                                          VkPhysicalDevice physicalDevice)
                    : instance(std::move(instance)), physicalDevice(physicalDevice)
                {
                }
            };
            auto physicalDeviceAndInstance =
                std::make_shared<PhysicalDeviceAndInstance>(instance, i);
            physicalDevice = std::shared_ptr<const VkPhysicalDevice>(
                physicalDeviceAndInstance, &physicalDeviceAndInstance->physicalDevice);
            break;
        }
        if(!physicalDevice)
            throw std::runtime_error("Vulkan: no viable physical devices found");
        VkDeviceQueueCreateInfo deviceQueueCreateInfo[1] = {};
        deviceQueueCreateInfo->sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        deviceQueueCreateInfo->queueFamilyIndex = queueFamilyIndex;
        deviceQueueCreateInfo->queueCount = 1;
        static const float queuePriority[] = {1.0f};
        deviceQueueCreateInfo->pQueuePriorities = &queuePriority[0];
        VkDeviceCreateInfo deviceCreateInfo{};
        deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        deviceCreateInfo.queueCreateInfoCount = 1;
        deviceCreateInfo.pQueueCreateInfos = deviceQueueCreateInfo;
        deviceCreateInfo.pEnabledFeatures = nullptr;
        static const char *const deviceExtensionNames[] = {
            VK_KHR_SWAPCHAIN_EXTENSION_NAME,
        };
        deviceCreateInfo.enabledExtensionCount = sizeof(deviceExtensionNames)/sizeof(deviceExtensionNames[0]);
        deviceCreateInfo.ppEnabledExtensionNames = deviceExtensionNames;
        {
            auto deviceHolder = std::make_shared<DeviceHolder>(instance);
            handleVulkanResult(
                vkCreateDevice(*physicalDevice, &deviceCreateInfo, nullptr, &deviceHolder->device),
                "vkCreateDevice");
            deviceHolder->loadDestroyFunction(this);
            device = std::shared_ptr<const VkDevice>(deviceHolder, &deviceHolder->device);
        }
        loadDeviceFunctions();
        throw std::runtime_error("VulkanDriver not finished");
#warning finish
    }
    void destroyGraphicsContext() noexcept
    {
#warning finish
        vkDeviceWaitIdle(*device); // ignore return value
        device = nullptr;
        physicalDevice = nullptr;
        instance = nullptr;
        unloadFunctions();
    }
    VkResult handleVulkanResult(VkResult result, const char *functionName)
    {
        if(result >= 0)
            return result;
        constexpr int VK_ERROR_FRAGMENTED_POOL = -12; // not all vulkan.h files have this
        switch(static_cast<int>(result))
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

std::shared_ptr<VulkanDriver::Implementation> VulkanDriver::createImplementation()
{
    return std::make_shared<Implementation>(this);
}

void VulkanDriver::renderFrame(std::shared_ptr<CommandBuffer> commandBuffer)
{
#warning finish
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
#warning finish
    return TextureId();
}

void VulkanDriver::setNewImageData(TextureId texture, const std::shared_ptr<const Image> &image)
{
#warning finish
}

std::shared_ptr<RenderBuffer> VulkanDriver::makeBuffer(
    const util::EnumArray<std::size_t, RenderLayer> &maximumSizes)
{
#warning finish
    return nullptr;
}

std::shared_ptr<CommandBuffer> VulkanDriver::makeCommandBuffer()
{
#warning finish
    return nullptr;
}

void VulkanDriver::setGraphicsContextRecreationNeeded() noexcept
{
#warning finish
}
}
}
}
}
#endif
