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
#include "../../logging/logging.h"
#include <cassert>
#include <cstdlib>
#include <memory>
#include <stdexcept>
#include <vector>
#include <sstream>
#include <cstdint>
#include <utility>
#include <mutex>
#include <type_traits>
#include "SDL_syswm.h"
#if defined(__ANDROID__)
#define VK_USE_PLATFORM_ANDROID_KHR
#elif defined(__linux__)
#define VK_USE_PLATFORM_XCB_KHR
#include <X11/Xlib-xcb.h>
#elif defined(_WIN32)
#define VK_USE_PLATFORM_WIN32_KHR
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
        virtual std::shared_ptr<const VkSurfaceKHR> createSurface(
            Implementation *implementation,
            const std::shared_ptr<const VkInstance> &instance,
            const SDL_SysWMinfo &wmInfo) const = 0;
    };
    struct InstanceHolder final
    {
        std::shared_ptr<void> vulkanLoader;
        PFN_vkDestroyInstance vkDestroyInstance = nullptr;
        VkInstance instance = VK_NULL_HANDLE;
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
            rt.instance = VK_NULL_HANDLE;
        }
        InstanceHolder &operator=(InstanceHolder rt)
        {
            vulkanLoader.swap(rt.vulkanLoader);
            std::swap(vkDestroyInstance, rt.vkDestroyInstance);
            std::swap(instance, rt.instance);
            return *this;
        }
    };
    struct SurfaceHolder final
    {
        std::shared_ptr<const VkInstance> instance;
        PFN_vkDestroySurfaceKHR vkDestroySurfaceKHR = nullptr;
        VkSurfaceKHR surface = VK_NULL_HANDLE;
        explicit SurfaceHolder(std::shared_ptr<const VkInstance> instance)
            : instance(std::move(instance))
        {
        }
        void loadDestroyFunction(Implementation *implementation)
        {
            assert(instance);
            implementation->loadInstanceFunction(
                vkDestroySurfaceKHR, *instance, "vkDestroySurfaceKHR");
        }
        ~SurfaceHolder()
        {
            if(!vkDestroySurfaceKHR) // surface could have been set to garbage by a failing
                // vkCreate*SurfaceKHR; check vkDestroySurfaceKHR instead
                return;
            vkDestroySurfaceKHR(*instance, surface, nullptr);
        }
        SurfaceHolder(SurfaceHolder &&rt)
            : instance(std::move(rt.instance)),
              vkDestroySurfaceKHR(rt.vkDestroySurfaceKHR),
              surface(rt.surface)
        {
            rt.vkDestroySurfaceKHR = nullptr;
            rt.surface = VK_NULL_HANDLE;
        }
        SurfaceHolder &operator=(SurfaceHolder rt)
        {
            instance.swap(rt.instance);
            std::swap(vkDestroySurfaceKHR, rt.vkDestroySurfaceKHR);
            std::swap(surface, rt.surface);
            return *this;
        }
    };
    struct DeviceHolder final
    {
        std::shared_ptr<const VkInstance> instance;
        PFN_vkDestroyDevice vkDestroyDevice = nullptr;
        VkDevice device = VK_NULL_HANDLE;
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
            rt.device = VK_NULL_HANDLE;
        }
        DeviceHolder &operator=(DeviceHolder rt)
        {
            instance.swap(rt.instance);
            std::swap(vkDestroyDevice, rt.vkDestroyDevice);
            std::swap(device, rt.device);
            return *this;
        }
    };
    template <typename T, typename DestroyFn>
    struct DeviceSubobjectHolder final
    {
        std::shared_ptr<const VkDevice> device;
        T subobject{};
        bool valid = false;
        DestroyFn destroyFn;
        explicit DeviceSubobjectHolder(std::shared_ptr<const VkDevice> device,
                                       DestroyFn &&destroyFn)
            : device(std::move(device)), destroyFn(std::move(destroyFn))
        {
        }
        ~DeviceSubobjectHolder()
        {
            if(valid)
                destroyFn(device, subobject);
        }
        DeviceSubobjectHolder(const DeviceSubobjectHolder &rt) = delete;
        DeviceSubobjectHolder &operator=(DeviceSubobjectHolder rt) = delete;
    };
    struct VulkanTextureImplementation final : public TextureImplementation
    {
#warning finish
        const std::size_t width;
        const std::size_t height;
        VulkanTextureImplementation(std::size_t width, std::size_t height)
            : width(width), height(height)
        {
        }
    };
    struct VulkanRenderBuffer final : public RenderBuffer
    {
#warning finish
    private:
        struct TriangleBuffer final
        {
            std::unique_ptr<Triangle[]> buffer;
            std::size_t bufferSize;
            std::size_t bufferUsed;
            TriangleBuffer(std::size_t size = 0)
                : buffer(size > 0 ? new Triangle[size] : nullptr), bufferSize(size), bufferUsed(0)
            {
            }
            std::size_t allocateSpace(std::size_t triangleCount) noexcept
            {
                constexprAssert(triangleCount <= bufferSize
                                && triangleCount + bufferUsed <= bufferSize);
                auto retval = bufferUsed;
                bufferUsed += triangleCount;
                return retval;
            }
            void append(const Triangle *triangles, std::size_t triangleCount) noexcept
            {
                std::size_t location = allocateSpace(triangleCount);
                for(std::size_t i = 0; i < triangleCount; i++)
                {
                    constexprAssert(triangles[i].texture.value == nullptr
                                    || dynamic_cast<const VulkanTextureImplementation *>(
                                           triangles[i].texture.value));
                    buffer[location + i] = triangles[i];
                }
            }
            void append(const Triangle &triangle) noexcept
            {
                append(&triangle, 1);
            }
            std::size_t sizeLeft() const noexcept
            {
                return bufferSize - bufferUsed;
            }
        };
        util::EnumArray<TriangleBuffer, RenderLayer> triangleBuffers;
        bool finished;

    public:
        VulkanRenderBuffer(const util::EnumArray<std::size_t, RenderLayer> &maximumSizes)
            : triangleBuffers(), finished(false)
        {
            for(RenderLayer renderLayer : util::EnumTraits<RenderLayer>::values)
            {
                triangleBuffers[renderLayer] = TriangleBuffer(maximumSizes[renderLayer]);
            }
        }
        bool isFinished() const noexcept
        {
            return finished;
        }
        virtual std::size_t getMaximumAdditionalSize(RenderLayer renderLayer) const
            noexcept override
        {
            return triangleBuffers[renderLayer].sizeLeft();
        }
        virtual void reserveAdditional(RenderLayer renderLayer,
                                       std::size_t howManyTriangles) override
        {
            constexprAssert(!finished);
        }
        virtual void appendTriangles(RenderLayer renderLayer,
                                     const Triangle *triangles,
                                     std::size_t triangleCount) override
        {
            constexprAssert(!finished);
            triangleBuffers[renderLayer].append(triangles, triangleCount);
        }
        virtual void appendTriangles(RenderLayer renderLayer,
                                     const Triangle *triangles,
                                     std::size_t triangleCount,
                                     const Transform &tform) override
        {
            constexprAssert(!finished);
            for(std::size_t i = 0; i < triangleCount; i++)
                triangleBuffers[renderLayer].append(transform(tform, triangles[i]));
        }
        virtual void appendBuffer(const ReadableRenderBuffer &buffer) override
        {
            constexprAssert(!finished);
            for(auto renderLayer : util::EnumTraits<RenderLayer>::values)
            {
                auto &triangleBuffer = triangleBuffers[renderLayer];
                std::size_t triangleCount = buffer.getTriangleCount(renderLayer);
                if(!triangleCount)
                    continue;
                std::size_t location = triangleBuffer.allocateSpace(triangleCount);
                buffer.readTriangles(renderLayer, &triangleBuffer.buffer[location], triangleCount);
                for(std::size_t i = 0; i < triangleCount; i++)
                {
                    constexprAssert(triangleBuffer.buffer[location + i].texture.value == nullptr
                                    || dynamic_cast<const VulkanTextureImplementation *>(
                                           triangleBuffer.buffer[location + i].texture.value));
                }
            }
        }
        virtual void appendBuffer(const ReadableRenderBuffer &buffer,
                                  const Transform &tform) override
        {
            constexprAssert(!finished);
            for(auto renderLayer : util::EnumTraits<RenderLayer>::values)
            {
                auto &triangleBuffer = triangleBuffers[renderLayer];
                std::size_t triangleCount = buffer.getTriangleCount(renderLayer);
                if(!triangleCount)
                    continue;
                std::size_t location = triangleBuffer.allocateSpace(triangleCount);
                buffer.readTriangles(renderLayer, &triangleBuffer.buffer[location], triangleCount);
                for(std::size_t i = 0; i < triangleCount; i++)
                {
                    constexprAssert(triangleBuffer.buffer[location + i].texture.value == nullptr
                                    || dynamic_cast<const VulkanTextureImplementation *>(
                                           triangleBuffer.buffer[location + i].texture.value));
                    triangleBuffer.buffer[location + i] =
                        transform(tform, triangleBuffer.buffer[location + i]);
                }
            }
        }
        virtual void finish() noexcept override
        {
            finished = true;
        }
    };
    struct VulkanCommandBuffer final : public CommandBuffer
    {
        std::shared_ptr<const VkDevice> device;
        std::shared_ptr<const VkCommandPool> commandPool;
        std::shared_ptr<const VkCommandBuffer> commandBuffer;
        std::vector<std::shared_ptr<RenderBuffer>> renderBuffers;
        bool finished = false;
        bool hasInitialClearColor = false;
        bool hasInitialClearDepth = false;
        bool hasAnyRenderCommands = false;
        ColorF clearColor{};
        virtual void appendClearCommand(bool colorFlag,
                                        bool depthFlag,
                                        const ColorF &backgroundColor) override
        {
            constexprAssert(!finished);
            if(!colorFlag && !depthFlag)
                return;
            if(!hasAnyRenderCommands)
            {
                if(colorFlag)
                {
                    hasInitialClearColor = true;
                    clearColor = backgroundColor;
                }
                if(depthFlag)
                {
                    hasInitialClearDepth = true;
                }
                return;
            }
#warning finish
            constexprAssert("not implemented");
            std::abort();
        }
        virtual void appendRenderCommand(const std::shared_ptr<RenderBuffer> &renderBuffer,
                                         const Transform &modelTransform,
                                         const Transform &viewTransform,
                                         const Transform &projectionTransform) override
        {
            constexprAssert(!finished);
            if(dynamic_cast<const EmptyRenderBuffer *>(renderBuffer.get()))
                return;
#warning finish
            constexprAssert(dynamic_cast<const VulkanRenderBuffer *>(renderBuffer.get()));
            constexprAssert(
                static_cast<const VulkanRenderBuffer *>(renderBuffer.get())->isFinished());
            renderBuffers.push_back(renderBuffer);
        }
        virtual void appendPresentCommandAndFinish() override
        {
            constexprAssert(!finished);
            finished = true;
#warning finish
        }
    };

public:
    VulkanDriver *const vulkanDriver;
    std::shared_ptr<void> vulkanLoader = nullptr;
    PFN_vkGetInstanceProcAddr vkGetInstanceProcAddr = nullptr;
    std::shared_ptr<const VkInstance> instance = nullptr;
    const WMHelper *wmHelper = nullptr;
    std::shared_ptr<const VkSurfaceKHR> surface = nullptr;
    VkPhysicalDeviceProperties physicalDeviceProperties;
    VkPhysicalDeviceFeatures physicalDeviceFeatures;
    std::uint32_t graphicsQueueFamilyIndex = 0;
    std::uint32_t presentQueueFamilyIndex = 0;
    std::shared_ptr<const VkPhysicalDevice> physicalDevice = nullptr;
    std::shared_ptr<const VkDevice> device = nullptr;
    std::shared_ptr<const VkQueue> graphicsQueue = nullptr;
    std::shared_ptr<const VkQueue> presentQueue = nullptr;
    std::shared_ptr<const VkSemaphore> imageAvailableSemaphore = nullptr;
    std::shared_ptr<const VkSemaphore> renderingFinishedSemaphore = nullptr;
    std::shared_ptr<const VkSwapchainKHR> swapchain = nullptr;
    std::vector<std::shared_ptr<const VkImage>> swapchainImages{};
    std::vector<std::shared_ptr<const VkImageView>> swapchainImageViews{};

public:
#define VULKAN_DRIVER_FUNCTIONS()                                              \
    VULKAN_DRIVER_GLOBAL_FUNCTION(vkEnumerateInstanceLayerProperties)          \
    VULKAN_DRIVER_GLOBAL_FUNCTION(vkEnumerateInstanceExtensionProperties)      \
    VULKAN_DRIVER_GLOBAL_FUNCTION(vkCreateInstance)                            \
    VULKAN_DRIVER_INSTANCE_FUNCTION(vkEnumeratePhysicalDevices)                \
    VULKAN_DRIVER_INSTANCE_FUNCTION(vkGetDeviceProcAddr)                       \
    VULKAN_DRIVER_INSTANCE_FUNCTION(vkGetPhysicalDeviceProperties)             \
    VULKAN_DRIVER_INSTANCE_FUNCTION(vkGetPhysicalDeviceFeatures)               \
    VULKAN_DRIVER_INSTANCE_FUNCTION(vkGetPhysicalDeviceQueueFamilyProperties)  \
    VULKAN_DRIVER_INSTANCE_FUNCTION(vkCreateDevice)                            \
    VULKAN_DRIVER_INSTANCE_FUNCTION(vkEnumerateDeviceExtensionProperties)      \
    VULKAN_DRIVER_INSTANCE_FUNCTION(vkGetPhysicalDeviceSurfaceSupportKHR)      \
    VULKAN_DRIVER_INSTANCE_FUNCTION(vkGetPhysicalDeviceSurfaceCapabilitiesKHR) \
    VULKAN_DRIVER_INSTANCE_FUNCTION(vkGetPhysicalDeviceSurfaceFormatsKHR)      \
    VULKAN_DRIVER_INSTANCE_FUNCTION(vkGetPhysicalDeviceSurfacePresentModesKHR) \
    VULKAN_DRIVER_DEVICE_FUNCTION(vkGetDeviceQueue)                            \
    VULKAN_DRIVER_DEVICE_FUNCTION(vkCreateSemaphore)                           \
    VULKAN_DRIVER_DEVICE_FUNCTION(vkDestroySemaphore)                          \
    VULKAN_DRIVER_DEVICE_FUNCTION(vkCreateSwapchainKHR)                        \
    VULKAN_DRIVER_DEVICE_FUNCTION(vkDestroySwapchainKHR)                       \
    VULKAN_DRIVER_DEVICE_FUNCTION(vkAcquireNextImageKHR)                       \
    VULKAN_DRIVER_DEVICE_FUNCTION(vkGetSwapchainImagesKHR)                     \
    VULKAN_DRIVER_DEVICE_FUNCTION(vkDestroyImageView)                          \
    VULKAN_DRIVER_DEVICE_FUNCTION(vkCreateImageView)                           \
    VULKAN_DRIVER_DEVICE_FUNCTION(vkCreateFence)                               \
    VULKAN_DRIVER_DEVICE_FUNCTION(vkDestroyFence)                              \
    VULKAN_DRIVER_DEVICE_FUNCTION(vkCreateCommandPool)                         \
    VULKAN_DRIVER_DEVICE_FUNCTION(vkDestroyCommandPool)                        \
    VULKAN_DRIVER_DEVICE_FUNCTION(vkDeviceWaitIdle)

#define VULKAN_DRIVER_GLOBAL_FUNCTION(name) PFN_##name name = nullptr;
#define VULKAN_DRIVER_INSTANCE_FUNCTION(name) PFN_##name name = nullptr;
#define VULKAN_DRIVER_DEVICE_FUNCTION(name) PFN_##name name = nullptr;
    VULKAN_DRIVER_FUNCTIONS()
#undef VULKAN_DRIVER_GLOBAL_FUNCTION
#undef VULKAN_DRIVER_INSTANCE_FUNCTION
#undef VULKAN_DRIVER_DEVICE_FUNCTION
public:
    void unloadFunctions()
    {
#define VULKAN_DRIVER_GLOBAL_FUNCTION(name) name = nullptr;
#define VULKAN_DRIVER_INSTANCE_FUNCTION(name) name = nullptr;
#define VULKAN_DRIVER_DEVICE_FUNCTION(name) name = nullptr;
        VULKAN_DRIVER_FUNCTIONS()
#undef VULKAN_DRIVER_GLOBAL_FUNCTION
#undef VULKAN_DRIVER_INSTANCE_FUNCTION
#undef VULKAN_DRIVER_DEVICE_FUNCTION
    }
    template <typename Fn>
    void loadGlobalFunction(Fn &fn, const char *name) const
    {
        fn = reinterpret_cast<Fn>(vkGetInstanceProcAddr(VK_NULL_HANDLE, name));
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
        VULKAN_DRIVER_FUNCTIONS()
#undef VULKAN_DRIVER_GLOBAL_FUNCTION
#undef VULKAN_DRIVER_INSTANCE_FUNCTION
#undef VULKAN_DRIVER_DEVICE_FUNCTION
    }
    void loadInstanceFunctions()
    {
        assert(instance != nullptr);
#define VULKAN_DRIVER_GLOBAL_FUNCTION(name)
#define VULKAN_DRIVER_INSTANCE_FUNCTION(name) loadInstanceFunction(name, #name);
#define VULKAN_DRIVER_DEVICE_FUNCTION(name)
        VULKAN_DRIVER_FUNCTIONS()
#undef VULKAN_DRIVER_GLOBAL_FUNCTION
#undef VULKAN_DRIVER_INSTANCE_FUNCTION
#undef VULKAN_DRIVER_DEVICE_FUNCTION
    }
    void loadDeviceFunctions()
    {
        assert(instance != nullptr);
#define VULKAN_DRIVER_GLOBAL_FUNCTION(name)
#define VULKAN_DRIVER_INSTANCE_FUNCTION(name)
#define VULKAN_DRIVER_DEVICE_FUNCTION(name) loadDeviceFunction(name, #name);
        VULKAN_DRIVER_FUNCTIONS()
#undef VULKAN_DRIVER_GLOBAL_FUNCTION
#undef VULKAN_DRIVER_INSTANCE_FUNCTION
#undef VULKAN_DRIVER_DEVICE_FUNCTION
    }
#undef VULKAN_DRIVER_FUNCTIONS

public:
    void loadLoader()
    {
        assert(!vulkanLoader);
#if defined(VK_USE_PLATFORM_XCB_KHR) || defined(VK_USE_PLATFORM_XLIB_KHR)
        const char *vulkanLoaderLibraryName = "libvulkan.so.1";
#elif defined(VK_USE_PLATFORM_WIN32_KHR)
        const char *vulkanLoaderLibraryName = "vulkan-1.dll";
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
        virtual std::shared_ptr<const VkSurfaceKHR> createSurface(
            Implementation *implementation,
            const std::shared_ptr<const VkInstance> &instance,
            const SDL_SysWMinfo &wmInfo) const override
        {
            struct Holder final
            {
                SurfaceHolder surfaceHolder;
                std::shared_ptr<void> xlibXCBLibrary;
                explicit Holder(std::shared_ptr<const VkInstance> instance)
                    : surfaceHolder(std::move(instance))
                {
                }
            };
            auto holder = std::make_shared<Holder>(instance);
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
            implementation->loadInstanceFunction(vkCreateXcbSurfaceKHR, "vkCreateXcbSurfaceKHR");
            VkXcbSurfaceCreateInfoKHR createInfo{};
            createInfo.sType = VK_STRUCTURE_TYPE_XCB_SURFACE_CREATE_INFO_KHR;
            createInfo.connection = getXCBConnection(wmInfo.info.x11.display);
            createInfo.window = static_cast<xcb_window_t>(wmInfo.info.x11.window);
            handleVulkanResult(vkCreateXcbSurfaceKHR(
                                   *instance, &createInfo, nullptr, &holder->surfaceHolder.surface),
                               "vkCreateXcbSurfaceKHR");
            holder->surfaceHolder.loadDestroyFunction(implementation);
            return std::shared_ptr<const VkSurfaceKHR>(holder, &holder->surfaceHolder.surface);
        }
    };
#endif
#ifdef VK_USE_PLATFORM_WIN32_KHR
    struct WMHelperWin32 final : public WMHelper
    {
        WMHelperWin32() : WMHelper(SDL_SYSWM_TYPE::SDL_SYSWM_WINDOWS)
        {
        }
        virtual void addEnabledInstanceExtensions(
            std::vector<const char *> &extensions) const override
        {
            extensions.push_back(VK_KHR_WIN32_SURFACE_EXTENSION_NAME);
        }
        virtual std::shared_ptr<const VkSurfaceKHR> createSurface(
            Implementation *implementation,
            const std::shared_ptr<const VkInstance> &instance,
            const SDL_SysWMinfo &wmInfo) const override
        {
            auto holder = std::make_shared<SurfaceHolder>(instance);
            PFN_vkCreateWin32SurfaceKHR vkCreateWin32SurfaceKHR = nullptr;
            implementation->loadInstanceFunction(vkCreateWin32SurfaceKHR,
                                                 "vkCreateWin32SurfaceKHR");
            VkWin32SurfaceCreateInfoKHR createInfo{};
            createInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
            createInfo.hinstance = ::GetWindowLongPtr(wmInfo.info.win.window, GWLP_HINSTANCE);
            createInfo.hwnd = wmInfo.info.win.window;
            handleVulkanResult(
                vkCreateWin32SurfaceKHR(*instance, &createInfo, nullptr, &holder->surface),
                "vkCreateXcbSurfaceKHR");
            holder->loadDestroyFunction(implementation);
            return std::shared_ptr<const VkSurfaceKHR>(holder, &holder->surface);
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
#ifdef VK_USE_PLATFORM_WIN32_KHR
            static const WMHelperWin32 helperWin32;
#endif
            static const WMHelper *const helpers[] = {
#ifdef VK_USE_PLATFORM_XCB_KHR
                &helperXCB,
#endif
#ifdef VK_USE_PLATFORM_WIN32_KHR
                &helperWin32,
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
    std::shared_ptr<const VkQueue> getQueue(std::shared_ptr<const VkDevice> device,
                                            std::uint32_t queueFamilyIndex,
                                            std::uint32_t queueIndex)
    {
        struct DeviceAndQueue
        {
            std::shared_ptr<const VkDevice> device;
            VkQueue queue;
        };
        auto deviceAndQueue = std::make_shared<DeviceAndQueue>();
        vkGetDeviceQueue(*device, queueFamilyIndex, queueIndex, &deviceAndQueue->queue);
        deviceAndQueue->device = std::move(device);
        return std::shared_ptr<const VkQueue>(deviceAndQueue, &deviceAndQueue->queue);
    }
    std::shared_ptr<const VkSemaphore> createSemaphore()
    {
        const auto vkDestroySemaphore = this->vkDestroySemaphore;
        auto destroyFn = [vkDestroySemaphore](const std::shared_ptr<const VkDevice> &device,
                                              VkSemaphore semaphore)
        {
            vkDestroySemaphore(*device, semaphore, nullptr);
        };
        auto holder = std::make_shared<DeviceSubobjectHolder<VkSemaphore, decltype(destroyFn)>>(
            device, std::move(destroyFn));
        VkSemaphoreCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
        handleVulkanResult(vkCreateSemaphore(*device, &createInfo, nullptr, &holder->subobject),
                           "");
        holder->valid = true;
        return std::shared_ptr<const VkSemaphore>(holder, &holder->subobject);
    }
    std::shared_ptr<const VkFence> createFence(bool signaled = false)
    {
        const auto vkDestroyFence = this->vkDestroyFence;
        auto destroyFn =
            [vkDestroyFence](const std::shared_ptr<const VkDevice> &device, VkFence fence)
        {
            vkDestroyFence(*device, fence, nullptr);
        };
        auto holder = std::make_shared<DeviceSubobjectHolder<VkFence, decltype(destroyFn)>>(
            device, std::move(destroyFn));
        VkFenceCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        if(signaled)
            createInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
        handleVulkanResult(vkCreateFence(*device, &createInfo, nullptr, &holder->subobject), "");
        holder->valid = true;
        return std::shared_ptr<const VkFence>(holder, &holder->subobject);
    }
    std::shared_ptr<const VkCommandPool> createCommandPool(bool transient,
                                                           bool resettable,
                                                           std::uint32_t queueFamilyIndex)
    {
        const auto vkDestroyCommandPool = this->vkDestroyCommandPool;
        auto destroyFn = [vkDestroyCommandPool](const std::shared_ptr<const VkDevice> &device,
                                                VkCommandPool fence)
        {
            vkDestroyCommandPool(*device, fence, nullptr);
        };
        auto holder = std::make_shared<DeviceSubobjectHolder<VkCommandPool, decltype(destroyFn)>>(
            device, std::move(destroyFn));
        VkCommandPoolCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        if(transient)
            createInfo.flags |= VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
        if(resettable)
            createInfo.flags |= VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        createInfo.queueFamilyIndex = queueFamilyIndex;
        handleVulkanResult(vkCreateCommandPool(*device, &createInfo, nullptr, &holder->subobject),
                           "");
        holder->valid = true;
        return std::shared_ptr<const VkCommandPool>(holder, &holder->subobject);
    }
    std::shared_ptr<const VkImageView> createImageView(std::shared_ptr<const VkImage> image,
                                                       VkImageViewType viewType,
                                                       VkFormat format,
                                                       VkImageAspectFlags aspectMask,
                                                       std::uint32_t baseMipLevel,
                                                       std::uint32_t levelCount,
                                                       std::uint32_t baseArrayLayer,
                                                       std::uint32_t layerCount)
    {
        auto vkDestroyImageView = this->vkDestroyImageView;
        auto destroyFn = [vkDestroyImageView](const std::shared_ptr<const VkDevice> &device,
                                              VkImageView imageView)
        {
            vkDestroyImageView(*device, imageView, nullptr);
        };
        typedef decltype(destroyFn) destroyFnType;
        struct Holder final
        {
            std::shared_ptr<const VkImage> image;
            DeviceSubobjectHolder<VkImageView, destroyFnType> subobject;
            Holder(std::shared_ptr<const VkImage> image,
                   std::shared_ptr<const VkDevice> device,
                   destroyFnType &&destroyFn)
                : image(std::move(image)), subobject(std::move(device), std::move(destroyFn))
            {
            }
        };
        VkImageViewCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        createInfo.image = *image;
        createInfo.viewType = viewType;
        createInfo.format = format;
        createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.subresourceRange.aspectMask = aspectMask;
        createInfo.subresourceRange.baseMipLevel = baseMipLevel;
        createInfo.subresourceRange.levelCount = levelCount;
        createInfo.subresourceRange.baseArrayLayer = baseArrayLayer;
        createInfo.subresourceRange.layerCount = layerCount;
        auto holder = std::make_shared<Holder>(std::move(image), device, std::move(destroyFn));
        handleVulkanResult(
            vkCreateImageView(*device, &createInfo, nullptr, &holder->subobject.subobject),
            "vkCreateImageView");
        holder->subobject.valid = true;
        return std::shared_ptr<const VkImageView>(holder, &holder->subobject.subobject);
    }
    void getSwapchainImages(VkFormat swapchainFormat)
    {
        struct SwapchainImagesHolder final
        {
            std::shared_ptr<const VkSwapchainKHR> swapchain;
            std::vector<VkImage> images;
        };
        auto holder = std::make_shared<SwapchainImagesHolder>();
        holder->swapchain = swapchain;
        std::uint32_t swapchainImageCount = 0;
        handleVulkanResult(
            vkGetSwapchainImagesKHR(*device, *swapchain, &swapchainImageCount, nullptr),
            "vkGetSwapchainImagesKHR");
        holder->images.resize(swapchainImageCount);
        swapchainImages.clear();
        swapchainImageViews.clear();
        handleVulkanResult(vkGetSwapchainImagesKHR(
                               *device, *swapchain, &swapchainImageCount, holder->images.data()),
                           "vkGetSwapchainImagesKHR");
        for(auto &image : holder->images)
            swapchainImages.push_back(std::shared_ptr<const VkImage>(holder, &image));
        for(auto &image : swapchainImages)
            swapchainImageViews.push_back(createImageView(image,
                                                          VK_IMAGE_VIEW_TYPE_2D,
                                                          swapchainFormat,
                                                          VK_IMAGE_ASPECT_COLOR_BIT,
                                                          0,
                                                          1,
                                                          0,
                                                          1));
    }
    void createNewSwapchain() // destroys old swapchain
    {
        handleVulkanResult(vkDeviceWaitIdle(*device), "vkDeviceWaitIdle");
        std::shared_ptr<const VkSwapchainKHR> oldSwapchain = std::move(swapchain);
        swapchain = nullptr;
        swapchainImages.clear();
        swapchainImageViews.clear();
        VkSurfaceCapabilitiesKHR surfaceCapabilities;
        handleVulkanResult(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(
                               *physicalDevice, *surface, &surfaceCapabilities),
                           "vkGetPhysicalDeviceSurfaceCapabilitiesKHR");
        std::uint32_t surfaceFormatsCount = 0;
        handleVulkanResult(vkGetPhysicalDeviceSurfaceFormatsKHR(
                               *physicalDevice, *surface, &surfaceFormatsCount, nullptr),
                           "vkGetPhysicalDeviceSurfaceFormatsKHR");
        if(surfaceFormatsCount == 0)
            throw std::runtime_error("VulkanDriver: no supported surface formats");
        std::vector<VkSurfaceFormatKHR> surfaceFormats;
        surfaceFormats.resize(surfaceFormatsCount);
        handleVulkanResult(
            vkGetPhysicalDeviceSurfaceFormatsKHR(
                *physicalDevice, *surface, &surfaceFormatsCount, surfaceFormats.data()),
            "vkGetPhysicalDeviceSurfaceFormatsKHR");
        std::uint32_t presentModesCount = 0;
        handleVulkanResult(vkGetPhysicalDeviceSurfacePresentModesKHR(
                               *physicalDevice, *surface, &presentModesCount, nullptr),
                           "vkGetPhysicalDeviceSurfacePresentModesKHR");
        std::vector<VkPresentModeKHR> presentModes;
        presentModes.resize(presentModesCount);
        handleVulkanResult(vkGetPhysicalDeviceSurfacePresentModesKHR(
                               *physicalDevice, *surface, &presentModesCount, presentModes.data()),
                           "vkGetPhysicalDeviceSurfacePresentModesKHR");
        VkPresentModeKHR presentMode = VK_PRESENT_MODE_FIFO_KHR;
        for(auto presentMode : presentModes)
        {
            std::string presentModeStr = "unknown";
            switch(presentMode)
            {
            case VK_PRESENT_MODE_IMMEDIATE_KHR:
                presentModeStr = "immediate";
                if(presentMode != VK_PRESENT_MODE_MAILBOX_KHR)
                    presentMode = VK_PRESENT_MODE_IMMEDIATE_KHR;
                break;
            case VK_PRESENT_MODE_MAILBOX_KHR:
                presentModeStr = "mailbox";
                presentMode = VK_PRESENT_MODE_MAILBOX_KHR;
                break;
            case VK_PRESENT_MODE_FIFO_KHR:
                presentModeStr = "fifo";
                break;
            case VK_PRESENT_MODE_FIFO_RELAXED_KHR:
                presentModeStr = "fifo relaxed";
                break;

            // so the compiler doesn't complain about missing enum values
            case VK_PRESENT_MODE_RANGE_SIZE_KHR:
            case VK_PRESENT_MODE_MAX_ENUM_KHR:
                break;
            }
            logging::log(logging::Level::Info, "VulkanDriver", "Present Mode: " + presentModeStr);
        }
        std::uint32_t swapchainImageCount = surfaceCapabilities.minImageCount + 1;
        if(surfaceCapabilities.maxImageCount
           && surfaceCapabilities.maxImageCount < swapchainImageCount)
            swapchainImageCount = surfaceCapabilities.maxImageCount;
        VkSurfaceFormatKHR surfaceFormat{};
        if(surfaceFormats.size() == 1 && surfaceFormats[0].format == VK_FORMAT_UNDEFINED)
        {
            surfaceFormat.format = VK_FORMAT_R8G8B8A8_UNORM;
            surfaceFormat.colorSpace = VK_COLORSPACE_SRGB_NONLINEAR_KHR;
        }
        else
        {
            surfaceFormat.format = VK_FORMAT_UNDEFINED;
            for(auto &format : surfaceFormats)
            {
                if(format.format == VK_FORMAT_R8G8B8A8_UNORM)
                {
                    surfaceFormat = format;
                    break;
                }
            }
            if(surfaceFormat.format == VK_FORMAT_UNDEFINED)
                surfaceFormat = surfaceFormats[0];
        }
        VkExtent2D swapchainExtent = surfaceCapabilities.currentExtent;
        if(swapchainExtent.width == static_cast<std::uint32_t>(-1)) // special extent
        {
            int w, h;
            SDL_GL_GetDrawableSize(vulkanDriver->getWindow(), &w, &h);
            swapchainExtent.width = w;
            swapchainExtent.height = h;
        }
        if(swapchainExtent.width == 0 || swapchainExtent.height == 0)
            return;
        VkImageUsageFlags imageUsageFlags =
            VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
        if((surfaceCapabilities.supportedUsageFlags & imageUsageFlags) != imageUsageFlags)
            throw std::runtime_error("VulkanDriver: surface doesn't support needed usage flags");
        VkSurfaceTransformFlagBitsKHR surfaceTransform = surfaceCapabilities.currentTransform;
        VkSwapchainCreateInfoKHR swapchainCreateInfo{};
        swapchainCreateInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        swapchainCreateInfo.surface = *surface;
        swapchainCreateInfo.minImageCount = swapchainImageCount;
        swapchainCreateInfo.imageFormat = surfaceFormat.format;
        swapchainCreateInfo.imageColorSpace = surfaceFormat.colorSpace;
        swapchainCreateInfo.imageExtent = swapchainExtent;
        swapchainCreateInfo.imageArrayLayers = 1;
        swapchainCreateInfo.imageUsage = imageUsageFlags;
        swapchainCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        swapchainCreateInfo.preTransform = surfaceTransform;
        swapchainCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        swapchainCreateInfo.presentMode = presentMode;
        swapchainCreateInfo.clipped = VK_TRUE;
        swapchainCreateInfo.oldSwapchain =
            oldSwapchain ? *oldSwapchain : static_cast<VkSwapchainKHR>(VK_NULL_HANDLE);
        const auto vkDestroySwapchainKHR = this->vkDestroySwapchainKHR;
        auto swapchainDeleter = [vkDestroySwapchainKHR](
            const std::shared_ptr<const VkDevice> &device, VkSwapchainKHR swapchain)
        {
            vkDestroySwapchainKHR(*device, swapchain, nullptr);
        };
        auto holder =
            std::make_shared<DeviceSubobjectHolder<VkSwapchainKHR, decltype(swapchainDeleter)>>(
                device, std::move(swapchainDeleter));
        handleVulkanResult(
            vkCreateSwapchainKHR(*device, &swapchainCreateInfo, nullptr, &holder->subobject),
            "vkCreateSwapchainKHR");
        holder->valid = true;
        swapchain = std::shared_ptr<const VkSwapchainKHR>(holder, &holder->subobject);
        try
        {
            getSwapchainImages(surfaceFormat.format);
        }
        catch(...)
        {
            swapchain = nullptr;
            throw;
        }
        oldSwapchain = nullptr;
    }
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
#if 1
#warning add code to check for layers
#endif
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
        surface = wmHelper->createSurface(this, instance, wmInfo);
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
            graphicsQueueFamilyIndex = queueFamiliesCount;
            presentQueueFamilyIndex = queueFamiliesCount;
            for(std::uint32_t j = 0; j < queueFamiliesCount; j++)
            {
                if(queueFamiliesProperties[j].queueCount == 0)
                    continue;
                if(queueFamiliesProperties[j].queueFlags & VK_QUEUE_GRAPHICS_BIT)
                    graphicsQueueFamilyIndex = j;
                VkBool32 supported = 0;
                handleVulkanResult(vkGetPhysicalDeviceSurfaceSupportKHR(i, j, *surface, &supported),
                                   "vkGetPhysicalDeviceSurfaceSupportKHR");
                if(supported)
                {
                    presentQueueFamilyIndex = j;
                    if(queueFamiliesProperties[j].queueFlags & VK_QUEUE_GRAPHICS_BIT)
                        break; // use this queue because it can present and do graphics
                }
            }
            if(graphicsQueueFamilyIndex == queueFamiliesCount) // no good queues found
                continue;
            if(presentQueueFamilyIndex == queueFamiliesCount) // no good queues found
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
        deviceQueueCreateInfo->queueFamilyIndex = graphicsQueueFamilyIndex;
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
        deviceCreateInfo.enabledExtensionCount =
            sizeof(deviceExtensionNames) / sizeof(deviceExtensionNames[0]);
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
        graphicsQueue = getQueue(device, graphicsQueueFamilyIndex, 0);
        if(graphicsQueueFamilyIndex == presentQueueFamilyIndex)
            presentQueue = graphicsQueue;
        else
            presentQueue = getQueue(device, presentQueueFamilyIndex, 0);
        imageAvailableSemaphore = createSemaphore();
        renderingFinishedSemaphore = createSemaphore();
        createNewSwapchain();
    }
    void destroyGraphicsContext() noexcept
    {
#warning finish
        vkDeviceWaitIdle(*device); // ignore return value
        swapchainImages.clear();
        swapchainImageViews.clear();
        swapchain = nullptr;
        imageAvailableSemaphore = nullptr;
        renderingFinishedSemaphore = nullptr;
        graphicsQueue = nullptr;
        presentQueue = nullptr;
        device = nullptr;
        surface = nullptr;
        physicalDevice = nullptr;
        instance = nullptr;
        unloadFunctions();
    }
    static VkResult handleVulkanResult(VkResult result, const char *functionName)
    {
        if(result >= 0)
            return result;
        constexpr long VK_ERROR_FRAGMENTED_POOL = -12; // not all vulkan.h files have this
        constexpr long VK_ERROR_SURFACE_LOST_KHR = -1000000000;
        constexpr long VK_ERROR_NATIVE_WINDOW_IN_USE_KHR = -1000000001;
        constexpr long VK_ERROR_OUT_OF_DATE_KHR = -1000001004;
        constexpr long VK_ERROR_INCOMPATIBLE_DISPLAY_KHR = -1000003001;
        constexpr long VK_ERROR_VALIDATION_FAILED_EXT = -1000011001;
        constexpr long VK_ERROR_INVALID_SHADER_NV = -1000012000;
        switch(static_cast<long>(result))
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
        case VK_ERROR_SURFACE_LOST_KHR:
            throw std::runtime_error(std::string("vulkan error: ") + functionName
                                     + " returned VK_ERROR_SURFACE_LOST_KHR");
        case VK_ERROR_NATIVE_WINDOW_IN_USE_KHR:
            throw std::runtime_error(std::string("vulkan error: ") + functionName
                                     + " returned VK_ERROR_NATIVE_WINDOW_IN_USE_KHR");
        case VK_ERROR_OUT_OF_DATE_KHR:
            throw std::runtime_error(std::string("vulkan error: ") + functionName
                                     + " returned VK_ERROR_OUT_OF_DATE_KHR");
        case VK_ERROR_INCOMPATIBLE_DISPLAY_KHR:
            throw std::runtime_error(std::string("vulkan error: ") + functionName
                                     + " returned VK_ERROR_INCOMPATIBLE_DISPLAY_KHR");
        case VK_ERROR_VALIDATION_FAILED_EXT:
            throw std::runtime_error(std::string("vulkan error: ") + functionName
                                     + " returned VK_ERROR_VALIDATION_FAILED_EXT");
        case VK_ERROR_INVALID_SHADER_NV:
            throw std::runtime_error(std::string("vulkan error: ") + functionName
                                     + " returned VK_ERROR_INVALID_SHADER_NV");
        default:
        {
            std::ostringstream ss;
            ss << "vulkan error: " << functionName << " returned " << result;
            throw std::runtime_error(ss.str());
        }
        }
    }
    void renderFrame(VulkanCommandBuffer &commandBuffer)
    {
        constexprAssert(commandBuffer.finished);
        if(!swapchain)
            return;
        std::uint32_t imageIndex = 0;
        VkResult acquireNextImageResult = vkAcquireNextImageKHR(
            *device, *swapchain, UINT64_MAX, *imageAvailableSemaphore, VK_NULL_HANDLE, &imageIndex);
        if(acquireNextImageResult == VK_ERROR_OUT_OF_DATE_KHR)
        {
            createNewSwapchain();
            return;
        }
        handleVulkanResult(acquireNextImageResult, "vkAcquireNextImageKHR");
        throw std::runtime_error("VulkanDriver not finished");
#warning finish
    }
};

std::shared_ptr<VulkanDriver::Implementation> VulkanDriver::createImplementation()
{
    return std::make_shared<Implementation>(this);
}

void VulkanDriver::renderFrame(std::shared_ptr<CommandBuffer> commandBuffer)
{
    constexprAssert(dynamic_cast<Implementation::VulkanCommandBuffer *>(commandBuffer.get()));
    implementation->renderFrame(static_cast<Implementation::VulkanCommandBuffer &>(*commandBuffer));
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
    return TextureId(new Implementation::VulkanTextureImplementation(image->width, image->height));
}

void VulkanDriver::setNewImageData(TextureId texture, const std::shared_ptr<const Image> &image)
{
#warning finish
}

std::shared_ptr<RenderBuffer> VulkanDriver::makeBuffer(
    const util::EnumArray<std::size_t, RenderLayer> &maximumSizes)
{
#warning finish
    return std::make_shared<Implementation::VulkanRenderBuffer>(maximumSizes);
}

std::shared_ptr<CommandBuffer> VulkanDriver::makeCommandBuffer()
{
#warning finish
    return std::make_shared<Implementation::VulkanCommandBuffer>();
}

void VulkanDriver::setGraphicsContextRecreationNeeded() noexcept
{
#warning finish
}
}
}
}
}
