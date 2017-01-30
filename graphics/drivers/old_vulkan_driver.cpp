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
#include "vulkan_driver.h"
#if 1
#warning finish VulkanDriver
#else
#include "../../logging/logging.h"
#include <cassert>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <stdexcept>
#include <cstddef>
#include <vector>
#include <sstream>
#include <cstdint>
#include <utility>
#include <mutex>
#include <deque>
#include <queue>
#include <tuple>
#include <chrono>
#include <cmath>
#include <unordered_map>
#include <map>
#include <set>
#include <list>
#include <unordered_set>
#include "../../util/atomic_shared_ptr.h"
#include "../../util/memory_manager.h"
#include "../shape/cube.h"
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
namespace
{
#include "vulkan.vert.h"
#include "vulkan.frag.h"
}
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
    class Buffer final
    {
        Buffer(const Buffer &) = delete;
        Buffer &operator=(const Buffer &) = delete;

    private:
        struct MappedBuffer final
        {
            MappedBuffer(const MappedBuffer &) = delete;
            MappedBuffer &operator=(const MappedBuffer &) = delete;
            std::shared_ptr<Buffer> buffer;
            void *memory;
            explicit MappedBuffer(std::shared_ptr<Buffer> buffer)
                : buffer(std::move(buffer)), memory(nullptr)
            {
            }
            void init(std::unique_lock<std::mutex> &lockIt)
            {
                try
                {
                    handleVulkanResult(
                        buffer->vkMapMemory(
                            *buffer->device, buffer->deviceMemory, 0, VK_WHOLE_SIZE, 0, &memory),
                        "vkMapMemory");
                }
                catch(...)
                {
                    memory = nullptr;
                    throw;
                }
            }
            ~MappedBuffer()
            {
                if(memory) // if map succeded
                {
                    std::unique_lock<std::mutex> lockIt(buffer->mapMutex);
                    buffer->vkUnmapMemory(*buffer->device, buffer->deviceMemory);
                }
            }
        };
        struct PrivateAccess final
        {
            friend class Buffer;

        private:
            PrivateAccess() = default;
        };

    private:
        const std::shared_ptr<const VkDevice> device;
        std::weak_ptr<Buffer> sharedThis;
        VkDeviceMemory deviceMemory;
        VkBuffer buffer;
        const PFN_vkDestroyBuffer vkDestroyBuffer;
        const PFN_vkFreeMemory vkFreeMemory;
        const PFN_vkMapMemory vkMapMemory;
        const PFN_vkUnmapMemory vkUnmapMemory;
        std::mutex mapMutex;
        std::weak_ptr<void *const> mappedMemory;
        const std::size_t bufferSize;

    private:
        void allocateBuffer(VkBufferUsageFlags usage,
                            const VkPhysicalDeviceMemoryProperties &physicalDeviceMemoryProperties,
                            PFN_vkCreateBuffer vkCreateBuffer,
                            PFN_vkAllocateMemory vkAllocateMemory,
                            PFN_vkGetBufferMemoryRequirements vkGetBufferMemoryRequirements,
                            PFN_vkBindBufferMemory vkBindBufferMemory)
        {
            assert(bufferSize);
            VkBufferCreateInfo createInfo{};
            createInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
            createInfo.size = bufferSize;
            createInfo.usage = usage;
            createInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE; // only one queue
            createInfo.queueFamilyIndexCount = 0;
            createInfo.pQueueFamilyIndices = nullptr;
            handleVulkanResult(vkCreateBuffer(*device, &createInfo, nullptr, &buffer),
                               "vkCreateBuffer");
            try
            {
                VkMemoryRequirements memoryRequirements;
                vkGetBufferMemoryRequirements(*device, buffer, &memoryRequirements);
                std::uint32_t memoryTypeIndex = 0;
                bool foundMemoryType = false;
                for(VkMemoryPropertyFlags requiredProperties :
                    std::initializer_list<VkMemoryPropertyFlags>{
                        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
                            | VK_MEMORY_PROPERTY_HOST_CACHED_BIT
                            | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_CACHED_BIT
                            | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                    })
                {
                    auto findResult = findMemoryType(memoryRequirements.memoryTypeBits,
                                                     requiredProperties,
                                                     physicalDeviceMemoryProperties);
                    if(std::get<1>(findResult))
                    {
                        foundMemoryType = true;
                        memoryTypeIndex = std::get<0>(findResult);
                        break;
                    }
                }
                if(!foundMemoryType)
                {
                    std::string message = "host-visible memory type not found for buffer";
                    logging::log(logging::Level::Fatal, "VulkanDriver", message);
                    throw std::runtime_error("VulkanDriver: " + std::move(message));
                }
                VkMemoryAllocateInfo allocateInfo{};
                allocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
                allocateInfo.allocationSize = memoryRequirements.size;
                allocateInfo.memoryTypeIndex = memoryTypeIndex;
                handleVulkanResult(vkAllocateMemory(*device, &allocateInfo, nullptr, &deviceMemory),
                                   "vkAllocateMemory");
                handleVulkanResult(vkBindBufferMemory(*device, buffer, deviceMemory, 0),
                                   "vkBindBufferMemory");
            }
            catch(...)
            {
                vkDestroyBuffer(*device, buffer, nullptr);
                throw;
            }
        }

    public:
        Buffer(std::shared_ptr<const VkDevice> device,
               std::size_t bufferSize,
               VkBufferUsageFlags usage,
               const VkPhysicalDeviceMemoryProperties &physicalDeviceMemoryProperties,
               PFN_vkCreateBuffer vkCreateBuffer,
               PFN_vkDestroyBuffer vkDestroyBuffer,
               PFN_vkAllocateMemory vkAllocateMemory,
               PFN_vkFreeMemory vkFreeMemory,
               PFN_vkMapMemory vkMapMemory,
               PFN_vkUnmapMemory vkUnmapMemory,
               PFN_vkGetBufferMemoryRequirements vkGetBufferMemoryRequirements,
               PFN_vkBindBufferMemory vkBindBufferMemory,
               PrivateAccess)
            : device(std::move(device)),
              sharedThis(),
              deviceMemory(VK_NULL_HANDLE),
              buffer(VK_NULL_HANDLE),
              vkDestroyBuffer(vkDestroyBuffer),
              vkFreeMemory(vkFreeMemory),
              vkMapMemory(vkMapMemory),
              vkUnmapMemory(vkUnmapMemory),
              mappedMemory(),
              bufferSize(bufferSize)
        {
            allocateBuffer(usage,
                           physicalDeviceMemoryProperties,
                           vkCreateBuffer,
                           vkAllocateMemory,
                           vkGetBufferMemoryRequirements,
                           vkBindBufferMemory);
        }
        static std::shared_ptr<Buffer> make(
            std::shared_ptr<const VkDevice> device,
            std::size_t bufferSize,
            VkBufferUsageFlags usage,
            const VkPhysicalDeviceMemoryProperties &physicalDeviceMemoryProperties,
            PFN_vkCreateBuffer vkCreateBuffer,
            PFN_vkDestroyBuffer vkDestroyBuffer,
            PFN_vkAllocateMemory vkAllocateMemory,
            PFN_vkFreeMemory vkFreeMemory,
            PFN_vkMapMemory vkMapMemory,
            PFN_vkUnmapMemory vkUnmapMemory,
            PFN_vkGetBufferMemoryRequirements vkGetBufferMemoryRequirements,
            PFN_vkBindBufferMemory vkBindBufferMemory)
        {
            auto retval = std::make_shared<Buffer>(std::move(device),
                                                   bufferSize,
                                                   usage,
                                                   physicalDeviceMemoryProperties,
                                                   vkCreateBuffer,
                                                   vkDestroyBuffer,
                                                   vkAllocateMemory,
                                                   vkFreeMemory,
                                                   vkMapMemory,
                                                   vkUnmapMemory,
                                                   vkGetBufferMemoryRequirements,
                                                   vkBindBufferMemory,
                                                   PrivateAccess());
            retval->sharedThis = retval;
            return retval;
        }
        ~Buffer()
        {
            assert(mappedMemory.expired());
            vkDestroyBuffer(*device, buffer, nullptr);
            vkFreeMemory(*device, deviceMemory, nullptr);
        }
        std::size_t size() const noexcept
        {
            return bufferSize;
        }
        std::shared_ptr<void *const> mapMemory()
        {
            std::unique_lock<std::mutex> lockIt(mapMutex);
            auto retval = mappedMemory.lock();
            if(retval)
                return retval;
            auto mappedBuffer = std::make_shared<MappedBuffer>(sharedThis.lock());
            mappedBuffer->init(lockIt);
            retval = std::shared_ptr<void *const>(mappedBuffer, &mappedBuffer->memory);
            mappedMemory = retval;
            return retval;
        }
        std::shared_ptr<const VkBuffer> getBuffer() const noexcept
        {
            return std::shared_ptr<const VkBuffer>(sharedThis.lock(), &buffer);
        }
    };
    template <VkBufferUsageFlags usage>
    struct BaseBufferMemoryManager final
    {
        Implementation *imp;
        typedef VkDeviceSize SizeType;
        typedef std::shared_ptr<Buffer> *BaseType;
        BaseBufferMemoryManager(Implementation *imp) : imp(imp)
        {
        }
        void free(BaseType buffer) noexcept
        {
            delete buffer;
        }
        BaseType allocate(SizeType size)
        {
            return new std::shared_ptr<Buffer>(Buffer::make(imp->atomicDevice.get(),
                                                            size,
                                                            usage,
                                                            imp->physicalDeviceMemoryProperties,
                                                            imp->vkCreateBuffer,
                                                            imp->vkDestroyBuffer,
                                                            imp->vkAllocateMemory,
                                                            imp->vkFreeMemory,
                                                            imp->vkMapMemory,
                                                            imp->vkUnmapMemory,
                                                            imp->vkGetBufferMemoryRequirements,
                                                            imp->vkBindBufferMemory));
        }
    };

    typedef util::MemoryManager<BaseBufferMemoryManager<VK_BUFFER_USAGE_VERTEX_BUFFER_BIT>,
                                static_cast<std::uint64_t>(1) << 20 /* 1MB */,
                                0x100 /* max nonCoherentAtomSize from Vulkan spec */>
        VertexBufferMemoryManager;
    typedef VertexBufferMemoryManager::AllocationReference VertexBufferAllocation;
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
    struct CommandBufferHolder final
    {
        std::shared_ptr<const VkDevice> device;
        std::shared_ptr<const VkCommandPool> commandPool;
        VkCommandBuffer commandBuffer = VK_NULL_HANDLE;
        PFN_vkFreeCommandBuffers vkFreeCommandBuffers = nullptr;
        CommandBufferHolder(std::shared_ptr<const VkDevice> device,
                            std::shared_ptr<const VkCommandPool> commandPool)
            : device(std::move(device)), commandPool(std::move(commandPool))
        {
        }
        ~CommandBufferHolder()
        {
            if(!vkFreeCommandBuffers) // commandBuffer could have been set to garbage by a failing
                // vkAllocateCommandBuffers; check vkFreeCommandBuffers instead
                return;
            vkFreeCommandBuffers(*device, *commandPool, 1, &commandBuffer);
        }
        CommandBufferHolder(const CommandBufferHolder &rt) = delete;
        CommandBufferHolder &operator=(CommandBufferHolder rt) = delete;
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
    struct VulkanVec4 final
    {
        float v[4];
        constexpr VulkanVec4() noexcept : v{}
        {
        }
        constexpr VulkanVec4(const ColorF &v) noexcept : v{v.red, v.green, v.blue, v.opacity}
        {
        }
        constexpr VulkanVec4(float v0, float v1, float v2, float v3) noexcept : v{v0, v1, v2, v3}
        {
        }
    };
    static_assert(sizeof(VulkanVec4) == 4 * sizeof(float), "");
    struct VulkanVec3 final
    {
        float v[3];
        constexpr VulkanVec3() noexcept : v{}
        {
        }
        constexpr VulkanVec3(const util::Vector3F &v) noexcept : v{v.x, v.y, v.z}
        {
        }
        constexpr VulkanVec3(float v0, float v1, float v2) noexcept : v{v0, v1, v2}
        {
        }
    };
    static_assert(sizeof(VulkanVec3) == 3 * sizeof(float), "");
    struct VulkanVec2 final
    {
        float v[2];
        constexpr VulkanVec2() noexcept : v{}
        {
        }
        constexpr VulkanVec2(const TextureCoordinates &v) noexcept : v{v.u, v.v}
        {
        }
        constexpr VulkanVec2(float v0, float v1) noexcept : v{v0, v1}
        {
        }
    };
    static_assert(sizeof(VulkanVec2) == 2 * sizeof(float), "");
    struct VulkanMat4 final
    {
        VulkanVec4 v[4];
        constexpr VulkanMat4() noexcept : v{}
        {
        }
        constexpr VulkanMat4(const util::Matrix4x4F &v) noexcept
            : v{VulkanVec4(v[0][0], v[0][1], v[0][2], v[0][3]),
                VulkanVec4(v[1][0], v[1][1], v[1][2], v[1][3]),
                VulkanVec4(v[2][0], v[2][1], v[2][2], v[2][3]),
                VulkanVec4(v[3][0], v[3][1], v[3][2], v[3][3])}
        {
        }
    };
    static_assert(sizeof(VulkanMat4) == 4 * sizeof(VulkanVec4), "");
    struct VulkanVertex final
    {
        VulkanVec3 position;
        static constexpr std::uint32_t positionLocation = 0;
        static constexpr VkFormat positionFormat = VK_FORMAT_R32G32B32_SFLOAT;
        VulkanVec4 color;
        static constexpr std::uint32_t colorLocation = 1;
        static constexpr VkFormat colorFormat = VK_FORMAT_R32G32B32A32_SFLOAT;
        VulkanVec2 textureCoordinates;
        static constexpr std::uint32_t textureCoordinatesLocation = 2;
        static constexpr VkFormat textureCoordinatesFormat = VK_FORMAT_R32G32_SFLOAT;
        constexpr VulkanVertex() noexcept : position(), color(), textureCoordinates()
        {
        }
        constexpr VulkanVertex(const Vertex &v) noexcept
            : position(v.getPosition()),
              color(v.getColor()),
              textureCoordinates(v.getTextureCoordinates())
        {
        }
        constexpr VulkanVertex(const VertexWithoutNormal &v) noexcept
            : position(v.getPosition()),
              color(v.getColor()),
              textureCoordinates(v.getTextureCoordinates())
        {
        }
    };
    struct VulkanTriangle final
    {
        VulkanVertex v[3];
        constexpr VulkanTriangle() noexcept : v{}
        {
        }
        constexpr VulkanTriangle(const Triangle &v) noexcept : v{VulkanVertex(v.vertices[0]),
                                                                 VulkanVertex(v.vertices[1]),
                                                                 VulkanVertex(v.vertices[2])}
        {
        }
        constexpr VulkanTriangle(const TriangleWithoutNormal &v) noexcept
            : v{VulkanVertex(v.vertices[0]),
                VulkanVertex(v.vertices[1]),
                VulkanVertex(v.vertices[2])}
        {
        }
    };
    static_assert(sizeof(VulkanTriangle) == 3 * sizeof(VulkanVertex), "");
    struct PushConstants final
    {
        // must match PushConstants in vulkan.vert
        VulkanMat4 transformMatrix;
    };
    static_assert(sizeof(PushConstants) <= 128,
                  "PushConstants is bigger than minimum guaranteed size");
    class TemporaryTriangleBuffer final
    {
        TemporaryTriangleBuffer(const TemporaryTriangleBuffer &) = delete;
        TemporaryTriangleBuffer &operator=(const TemporaryTriangleBuffer &) = delete;

    private:
        static const std::size_t localBufferSize = 16;
        TriangleWithoutNormal localBuffer[localBufferSize];
        TriangleWithoutNormal *buffer;
        std::size_t bufferSize;

    public:
        TemporaryTriangleBuffer() noexcept : localBuffer{},
                                             buffer(&localBuffer[0]),
                                             bufferSize(localBufferSize)
        {
        }
        ~TemporaryTriangleBuffer()
        {
            freeHeapMemory();
        }
        void freeHeapMemory() noexcept
        {
            if(buffer != &localBuffer[0])
            {
                delete[] buffer;
                buffer = &localBuffer[0];
                bufferSize = localBufferSize;
            }
        }
        void requireSize(std::size_t requiredSize)
        {
            if(requiredSize > bufferSize)
            {
                freeHeapMemory();
                bufferSize = requiredSize;
                buffer = new TriangleWithoutNormal[requiredSize];
            }
        }
        TriangleWithoutNormal *get() const noexcept
        {
            return buffer;
        }
    };
    struct VulkanRenderBuffer final : public RenderBuffer
    {
#warning finish
        struct TriangleBuffer final
        {
            std::unique_ptr<VulkanTriangle[]> buffer;
            std::size_t bufferSize;
            std::size_t bufferUsed;
            std::size_t finalBufferOffset;
            TriangleBuffer(std::size_t size = 0)
                : buffer(size > 0 ? new VulkanTriangle[size] : nullptr),
                  bufferSize(size),
                  bufferUsed(0),
                  finalBufferOffset()
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
        std::shared_ptr<const VkDevice> device;
        VertexBufferAllocation buffer;
        std::shared_ptr<void *const> mappedBuffer;
        VulkanRenderBuffer(const util::EnumArray<std::size_t, RenderLayer> &maximumSizes)
            : triangleBuffers(), finished(false)
        {
            for(RenderLayer renderLayer : util::EnumTraits<RenderLayer>::values)
            {
                triangleBuffers[renderLayer] = TriangleBuffer(maximumSizes[renderLayer]);
            }
        }
        void attachDevice(const std::shared_ptr<const VkDevice> &device,
                          const std::shared_ptr<Implementation> &imp)
        {
            if(!device)
                return;
            if(this->device == device && buffer != nullptr)
                return;
            std::size_t bufferSize = 0;
            for(RenderLayer renderLayer : util::EnumTraits<RenderLayer>::values)
            {
                bufferSize += triangleBuffers[renderLayer].bufferSize * sizeof(VulkanTriangle);
            }
            if(bufferSize == 0)
                return;
            this->device = device;
            mappedBuffer = nullptr;
            buffer = nullptr;
            buffer = imp->vertexBufferMemoryManager.allocate(bufferSize);
            mappedBuffer = (*buffer.getBase())->mapMemory();
            std::size_t bufferPartStartOffset = buffer.getOffset();
            for(RenderLayer renderLayer : util::EnumTraits<RenderLayer>::values)
            {
                triangleBuffers[renderLayer].finalBufferOffset = bufferPartStartOffset;
                if(triangleBuffers[renderLayer].bufferUsed)
                    std::memcpy(static_cast<char *>(*mappedBuffer)
                                    + triangleBuffers[renderLayer].finalBufferOffset,
                                triangleBuffers[renderLayer].buffer.get(),
                                triangleBuffers[renderLayer].bufferUsed * sizeof(VulkanTriangle));
                bufferPartStartOffset +=
                    triangleBuffers[renderLayer].bufferSize * sizeof(VulkanTriangle);
            }
            if(finished)
                mappedBuffer = nullptr;
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
            auto &triangleBuffer = triangleBuffers[renderLayer];
            std::size_t initialUsedCount = triangleBuffer.bufferUsed;
            triangleBuffer.append(triangles, triangleCount);
            if(!mappedBuffer)
                return;
            std::memcpy(static_cast<char *>(*mappedBuffer) + triangleBuffer.finalBufferOffset
                            + initialUsedCount * sizeof(VulkanTriangle),
                        triangleBuffer.buffer.get() + initialUsedCount,
                        triangleBuffer.bufferUsed * sizeof(VulkanTriangle));
        }
        virtual void appendTriangles(RenderLayer renderLayer,
                                     const Triangle *triangles,
                                     std::size_t triangleCount,
                                     const Transform &tform) override
        {
            constexprAssert(!finished);
            auto &triangleBuffer = triangleBuffers[renderLayer];
            std::size_t initialUsedCount = triangleBuffer.bufferUsed;
            for(std::size_t i = 0; i < triangleCount; i++)
                triangleBuffer.append(transform(tform, triangles[i]));
            if(!mappedBuffer)
                return;
            std::memcpy(static_cast<char *>(*mappedBuffer) + triangleBuffer.finalBufferOffset
                            + initialUsedCount * sizeof(VulkanTriangle),
                        triangleBuffer.buffer.get() + initialUsedCount,
                        triangleBuffer.bufferUsed * sizeof(VulkanTriangle));
        }
        virtual void appendBuffer(const ReadableRenderBuffer &buffer) override
        {
            constexprAssert(!finished);
            TemporaryTriangleBuffer tempBuffer;
            for(auto renderLayer : util::EnumTraits<RenderLayer>::values)
            {
                auto &triangleBuffer = triangleBuffers[renderLayer];
                std::size_t initialUsedCount = triangleBuffer.bufferUsed;
                std::size_t triangleCount = buffer.getTriangleCount(renderLayer);
                if(!triangleCount)
                    continue;
                std::size_t location = triangleBuffer.allocateSpace(triangleCount);
                tempBuffer.requireSize(triangleCount);
                buffer.readTriangles(renderLayer, tempBuffer.get(), triangleCount);
                for(std::size_t i = 0; i < triangleCount; i++)
                {
                    constexprAssert(tempBuffer.get()[i].texture.value == nullptr
                                    || dynamic_cast<const VulkanTextureImplementation *>(
                                           tempBuffer.get()[i].texture.value));
                    triangleBuffer.buffer[i + location] = tempBuffer.get()[i];
                }
                if(!mappedBuffer)
                    continue;
                std::memcpy(static_cast<char *>(*mappedBuffer) + triangleBuffer.finalBufferOffset
                                + initialUsedCount * sizeof(VulkanTriangle),
                            triangleBuffer.buffer.get() + initialUsedCount,
                            triangleBuffer.bufferUsed * sizeof(VulkanTriangle));
            }
        }
        virtual void appendBuffer(const ReadableRenderBuffer &buffer,
                                  const Transform &tform) override
        {
            constexprAssert(!finished);
            TemporaryTriangleBuffer tempBuffer;
            for(auto renderLayer : util::EnumTraits<RenderLayer>::values)
            {
                auto &triangleBuffer = triangleBuffers[renderLayer];
                std::size_t initialUsedCount = triangleBuffer.bufferUsed;
                std::size_t triangleCount = buffer.getTriangleCount(renderLayer);
                if(!triangleCount)
                    continue;
                std::size_t location = triangleBuffer.allocateSpace(triangleCount);
                tempBuffer.requireSize(triangleCount);
                buffer.readTriangles(renderLayer, tempBuffer.get(), triangleCount);
                for(std::size_t i = 0; i < triangleCount; i++)
                {
                    constexprAssert(tempBuffer.get()[i].texture.value == nullptr
                                    || dynamic_cast<const VulkanTextureImplementation *>(
                                           tempBuffer.get()[i].texture.value));
                    triangleBuffer.buffer[i + location] = transform(tform, tempBuffer.get()[i]);
                }
                if(!mappedBuffer)
                    continue;
                std::memcpy(static_cast<char *>(*mappedBuffer) + triangleBuffer.finalBufferOffset
                                + initialUsedCount * sizeof(VulkanTriangle),
                            triangleBuffer.buffer.get() + initialUsedCount,
                            triangleBuffer.bufferUsed * sizeof(VulkanTriangle));
            }
        }
        virtual void finish() noexcept override
        {
            finished = true;
            mappedBuffer = nullptr;
        }
    };
    struct VulkanCommandBuffer final : public CommandBuffer
    {
        std::shared_ptr<const VkDevice> device;
        std::shared_ptr<const VkCommandPool> commandPool;
        std::shared_ptr<const VkCommandBuffer> commandBuffer;
        std::vector<VertexBufferAllocation> heldVertexBufferAllocations;
        std::vector<std::shared_ptr<const void>> heldSharedPtrs;
        std::shared_ptr<Implementation> imp;
        std::shared_ptr<const VkRenderPass> renderPass;
        PFN_vkEndCommandBuffer vkEndCommandBuffer;
        bool finished = false;
        bool hasInitialClearColor = false;
        bool hasInitialClearDepth = false;
        bool hasAnyRenderCommands = false;
        ColorF clearColor{};
        explicit VulkanCommandBuffer(std::shared_ptr<const VkDevice> deviceIn,
                                     const std::shared_ptr<Implementation> &imp)
            : device(std::move(deviceIn)), imp(imp), vkEndCommandBuffer(imp->vkEndCommandBuffer)
        {
            commandPool =
                imp->createCommandPool(device, false, false, imp->graphicsQueueFamilyIndex);
            commandBuffer =
                imp->allocateCommandBuffer(device, commandPool, VK_COMMAND_BUFFER_LEVEL_SECONDARY);
            VkCommandBufferInheritanceInfo inheritanceInfo{};
            inheritanceInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO;
            inheritanceInfo.renderPass = VK_NULL_HANDLE;
            inheritanceInfo.subpass = 0;
            inheritanceInfo.framebuffer = VK_NULL_HANDLE;
            inheritanceInfo.occlusionQueryEnable = VK_FALSE;
            inheritanceInfo.queryFlags = 0;
            inheritanceInfo.pipelineStatistics = 0;
            VkCommandBufferBeginInfo commandBufferBeginInfo{};
            commandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
            commandBufferBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
            commandBufferBeginInfo.pInheritanceInfo = &inheritanceInfo;
            handleVulkanResult(imp->vkBeginCommandBuffer(*commandBuffer, &commandBufferBeginInfo),
                               "vkBeginCommandBuffer");
        }
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
            constexprAssert(hasInitialClearColor && hasInitialClearDepth);
            constexprAssert(dynamic_cast<VulkanRenderBuffer *>(renderBuffer.get()));
            VulkanRenderBuffer *vulkanRenderBuffer =
                static_cast<VulkanRenderBuffer *>(renderBuffer.get());
            constexprAssert(vulkanRenderBuffer->isFinished());
#warning finish
            vulkanRenderBuffer->attachDevice(device, imp);
            heldVertexBufferAllocations.push_back(vulkanRenderBuffer->buffer);
        }
        virtual void appendPresentCommandAndFinish() override
        {
            constexprAssert(!finished);
            constexprAssert(hasInitialClearColor && hasInitialClearDepth);
            handleVulkanResult(vkEndCommandBuffer(*commandBuffer), "vkEndCommandBuffer");
            finished = true;
#warning finish
        }
    };
    struct FrameObjects final
    {
        const std::shared_ptr<const VkFence> fence;
        std::vector<std::shared_ptr<const void>> sharedPointers;
        std::vector<VertexBufferAllocation> vertexBufferAllocations;
        explicit FrameObjects(std::shared_ptr<const VkFence> fence)
            : fence(std::move(fence)), sharedPointers(), vertexBufferAllocations()
        {
        }
        void addObject(std::shared_ptr<const void> v)
        {
            sharedPointers.push_back(std::move(v));
        }
        void addObject(VertexBufferAllocation v)
        {
            vertexBufferAllocations.push_back(std::move(v));
        }
        void resetObjects()
        {
            sharedPointers.clear();
            vertexBufferAllocations.clear();
        }
    };
    struct AtomicDevice final
    {
        util::atomic_shared_ptr<const VkDevice> atomicDevice{};
        void set(std::shared_ptr<const VkDevice> device) noexcept
        {
            atomicDevice.store(std::move(device), std::memory_order_release);
        }
        std::shared_ptr<const VkDevice> get() const noexcept
        {
            return atomicDevice.load(std::memory_order_acquire);
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
    VkPhysicalDeviceMemoryProperties physicalDeviceMemoryProperties;
    VkPhysicalDeviceFeatures physicalDeviceFeatures;
    std::uint32_t graphicsQueueFamilyIndex = 0;
    std::uint32_t presentQueueFamilyIndex = 0;
    std::shared_ptr<const VkPhysicalDevice> physicalDevice = nullptr;
    AtomicDevice atomicDevice{};
    std::shared_ptr<const VkQueue> graphicsQueue = nullptr;
    std::shared_ptr<const VkQueue> presentQueue = nullptr;
    std::shared_ptr<const VkShaderModule> vertexShaderModule = nullptr;
    std::shared_ptr<const VkShaderModule> fragmentShaderModule = nullptr;
    std::shared_ptr<const VkSemaphore> imageAvailableSemaphore = nullptr;
    std::shared_ptr<const VkSemaphore> renderingFinishedSemaphore = nullptr;
    std::shared_ptr<const VkPipelineCache> pipelineCache = nullptr;
    std::shared_ptr<const VkSwapchainKHR> swapchain = nullptr;
    std::vector<std::shared_ptr<const VkImage>> swapchainImages{};
    std::vector<std::shared_ptr<const VkImageView>> swapchainImageViews{};
    std::deque<FrameObjects> frames;
    VkFormat renderPipelineFormat = VK_FORMAT_UNDEFINED;
    std::shared_ptr<const VkRenderPass> renderPass;
    std::shared_ptr<const VkPipelineLayout> renderPipelineLayout;
    std::shared_ptr<const VkPipeline> renderPipeline;
    VkExtent2D swapchainExtent{};
    VertexBufferMemoryManager vertexBufferMemoryManager;

public:
#define VULKAN_DRIVER_END()
#define VULKAN_DRIVER_FUNCTIONS()                                              \
    VULKAN_DRIVER_DEVICE_FUNCTION(vkAcquireNextImageKHR)                       \
    VULKAN_DRIVER_DEVICE_FUNCTION(vkAllocateCommandBuffers)                    \
    VULKAN_DRIVER_DEVICE_FUNCTION(vkAllocateMemory)                            \
    VULKAN_DRIVER_DEVICE_FUNCTION(vkBeginCommandBuffer)                        \
    VULKAN_DRIVER_DEVICE_FUNCTION(vkBindBufferMemory)                          \
    VULKAN_DRIVER_DEVICE_FUNCTION(vkCmdBeginRenderPass)                        \
    VULKAN_DRIVER_DEVICE_FUNCTION(vkCmdBindPipeline)                           \
    VULKAN_DRIVER_DEVICE_FUNCTION(vkCmdBindVertexBuffers)                      \
    VULKAN_DRIVER_DEVICE_FUNCTION(vkCmdDraw)                                   \
    VULKAN_DRIVER_DEVICE_FUNCTION(vkCmdEndRenderPass)                          \
    VULKAN_DRIVER_DEVICE_FUNCTION(vkCmdPushConstants)                          \
    VULKAN_DRIVER_DEVICE_FUNCTION(vkCmdSetScissor)                             \
    VULKAN_DRIVER_DEVICE_FUNCTION(vkCmdSetViewport)                            \
    VULKAN_DRIVER_DEVICE_FUNCTION(vkCreateBuffer)                              \
    VULKAN_DRIVER_DEVICE_FUNCTION(vkCreateCommandPool)                         \
    VULKAN_DRIVER_DEVICE_FUNCTION(vkCreateFence)                               \
    VULKAN_DRIVER_DEVICE_FUNCTION(vkCreateFramebuffer)                         \
    VULKAN_DRIVER_DEVICE_FUNCTION(vkCreateGraphicsPipelines)                   \
    VULKAN_DRIVER_DEVICE_FUNCTION(vkCreateImageView)                           \
    VULKAN_DRIVER_DEVICE_FUNCTION(vkCreatePipelineCache)                       \
    VULKAN_DRIVER_DEVICE_FUNCTION(vkCreatePipelineLayout)                      \
    VULKAN_DRIVER_DEVICE_FUNCTION(vkCreateRenderPass)                          \
    VULKAN_DRIVER_DEVICE_FUNCTION(vkCreateSemaphore)                           \
    VULKAN_DRIVER_DEVICE_FUNCTION(vkCreateShaderModule)                        \
    VULKAN_DRIVER_DEVICE_FUNCTION(vkCreateSwapchainKHR)                        \
    VULKAN_DRIVER_DEVICE_FUNCTION(vkDestroyBuffer)                             \
    VULKAN_DRIVER_DEVICE_FUNCTION(vkDestroyCommandPool)                        \
    VULKAN_DRIVER_DEVICE_FUNCTION(vkDestroyFence)                              \
    VULKAN_DRIVER_DEVICE_FUNCTION(vkDestroyFramebuffer)                        \
    VULKAN_DRIVER_DEVICE_FUNCTION(vkDestroyImageView)                          \
    VULKAN_DRIVER_DEVICE_FUNCTION(vkDestroyPipeline)                           \
    VULKAN_DRIVER_DEVICE_FUNCTION(vkDestroyPipelineCache)                      \
    VULKAN_DRIVER_DEVICE_FUNCTION(vkDestroyPipelineLayout)                     \
    VULKAN_DRIVER_DEVICE_FUNCTION(vkDestroyRenderPass)                         \
    VULKAN_DRIVER_DEVICE_FUNCTION(vkDestroySemaphore)                          \
    VULKAN_DRIVER_DEVICE_FUNCTION(vkDestroyShaderModule)                       \
    VULKAN_DRIVER_DEVICE_FUNCTION(vkDestroySwapchainKHR)                       \
    VULKAN_DRIVER_DEVICE_FUNCTION(vkDeviceWaitIdle)                            \
    VULKAN_DRIVER_DEVICE_FUNCTION(vkEndCommandBuffer)                          \
    VULKAN_DRIVER_DEVICE_FUNCTION(vkFreeCommandBuffers)                        \
    VULKAN_DRIVER_DEVICE_FUNCTION(vkFreeMemory)                                \
    VULKAN_DRIVER_DEVICE_FUNCTION(vkGetBufferMemoryRequirements)               \
    VULKAN_DRIVER_DEVICE_FUNCTION(vkGetDeviceQueue)                            \
    VULKAN_DRIVER_DEVICE_FUNCTION(vkGetFenceStatus)                            \
    VULKAN_DRIVER_DEVICE_FUNCTION(vkGetPipelineCacheData)                      \
    VULKAN_DRIVER_DEVICE_FUNCTION(vkGetSwapchainImagesKHR)                     \
    VULKAN_DRIVER_DEVICE_FUNCTION(vkMapMemory)                                 \
    VULKAN_DRIVER_DEVICE_FUNCTION(vkQueuePresentKHR)                           \
    VULKAN_DRIVER_DEVICE_FUNCTION(vkQueueSubmit)                               \
    VULKAN_DRIVER_DEVICE_FUNCTION(vkResetFences)                               \
    VULKAN_DRIVER_DEVICE_FUNCTION(vkUnmapMemory)                               \
    VULKAN_DRIVER_DEVICE_FUNCTION(vkWaitForFences)                             \
    VULKAN_DRIVER_GLOBAL_FUNCTION(vkCreateInstance)                            \
    VULKAN_DRIVER_GLOBAL_FUNCTION(vkEnumerateInstanceExtensionProperties)      \
    VULKAN_DRIVER_GLOBAL_FUNCTION(vkEnumerateInstanceLayerProperties)          \
    VULKAN_DRIVER_INSTANCE_FUNCTION(vkCreateDevice)                            \
    VULKAN_DRIVER_INSTANCE_FUNCTION(vkEnumerateDeviceExtensionProperties)      \
    VULKAN_DRIVER_INSTANCE_FUNCTION(vkEnumeratePhysicalDevices)                \
    VULKAN_DRIVER_INSTANCE_FUNCTION(vkGetDeviceProcAddr)                       \
    VULKAN_DRIVER_INSTANCE_FUNCTION(vkGetPhysicalDeviceFeatures)               \
    VULKAN_DRIVER_INSTANCE_FUNCTION(vkGetPhysicalDeviceMemoryProperties)       \
    VULKAN_DRIVER_INSTANCE_FUNCTION(vkGetPhysicalDeviceProperties)             \
    VULKAN_DRIVER_INSTANCE_FUNCTION(vkGetPhysicalDeviceQueueFamilyProperties)  \
    VULKAN_DRIVER_INSTANCE_FUNCTION(vkGetPhysicalDeviceSurfaceCapabilitiesKHR) \
    VULKAN_DRIVER_INSTANCE_FUNCTION(vkGetPhysicalDeviceSurfaceFormatsKHR)      \
    VULKAN_DRIVER_INSTANCE_FUNCTION(vkGetPhysicalDeviceSurfacePresentModesKHR) \
    VULKAN_DRIVER_INSTANCE_FUNCTION(vkGetPhysicalDeviceSurfaceSupportKHR)      \
    VULKAN_DRIVER_END()

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
    void loadDeviceFunctions(const std::shared_ptr<const VkDevice> &device)
    {
        assert(instance != nullptr);
#define VULKAN_DRIVER_GLOBAL_FUNCTION(name)
#define VULKAN_DRIVER_INSTANCE_FUNCTION(name)
#define VULKAN_DRIVER_DEVICE_FUNCTION(name) loadDeviceFunction(name, *device, #name);
        VULKAN_DRIVER_FUNCTIONS()
#undef VULKAN_DRIVER_GLOBAL_FUNCTION
#undef VULKAN_DRIVER_INSTANCE_FUNCTION
#undef VULKAN_DRIVER_DEVICE_FUNCTION
    }
#undef VULKAN_DRIVER_FUNCTIONS
#undef VULKAN_DRIVER_END

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
    explicit Implementation(VulkanDriver *vulkanDriver)
        : vulkanDriver(vulkanDriver), vertexBufferMemoryManager(this)
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
    static std::pair<std::uint32_t, bool> findMemoryType(
        std::uint32_t memoryTypeBits,
        VkMemoryPropertyFlags requiredProperties,
        const VkPhysicalDeviceMemoryProperties &physicalDeviceMemoryProperties) noexcept
    {
        for(std::uint32_t i = 0; i < physicalDeviceMemoryProperties.memoryTypeCount; i++)
            if(memoryTypeBits & (static_cast<std::uint32_t>(1) << i))
                if((physicalDeviceMemoryProperties.memoryTypes[i].propertyFlags
                    & requiredProperties) == requiredProperties)
                    return {i, true};
        return {0, false};
    }
    std::pair<std::uint32_t, bool> findMemoryType(std::uint32_t memoryTypeBits,
                                                  VkMemoryPropertyFlags requiredProperties) const
        noexcept
    {
        return findMemoryType(memoryTypeBits, requiredProperties, physicalDeviceMemoryProperties);
    }
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
    std::shared_ptr<const VkSemaphore> createSemaphore(
        const std::shared_ptr<const VkDevice> &device)
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
                           "vkCreateSemaphore");
        holder->valid = true;
        return std::shared_ptr<const VkSemaphore>(holder, &holder->subobject);
    }
    std::shared_ptr<const VkPipelineCache> createPipelineCache(
        const std::shared_ptr<const VkDevice> &device,
        const void *initialData,
        std::size_t initialDataSize)
    {
        const auto vkDestroyPipelineCache = this->vkDestroyPipelineCache;
        auto destroyFn = [vkDestroyPipelineCache](const std::shared_ptr<const VkDevice> &device,
                                                  VkPipelineCache pipelineCache)
        {
            vkDestroyPipelineCache(*device, pipelineCache, nullptr);
        };
        auto holder = std::make_shared<DeviceSubobjectHolder<VkPipelineCache, decltype(destroyFn)>>(
            device, std::move(destroyFn));
        VkPipelineCacheCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
        createInfo.initialDataSize = initialDataSize;
        createInfo.pInitialData = initialData;
        handleVulkanResult(vkCreatePipelineCache(*device, &createInfo, nullptr, &holder->subobject),
                           "vkCreatePipelineCache");
        holder->valid = true;
        return std::shared_ptr<const VkPipelineCache>(holder, &holder->subobject);
    }
    std::shared_ptr<const VkPipelineCache> createPipelineCache(
        const std::shared_ptr<const VkDevice> &device)
    {
        return createPipelineCache(device, nullptr, 0);
    }
    std::shared_ptr<const VkPipelineCache> createPipelineCache(
        const std::shared_ptr<const VkDevice> &device,
        const std::vector<unsigned char> &initialData)
    {
        return createPipelineCache(
            device, static_cast<const void *>(initialData.data()), initialData.size());
    }
    std::shared_ptr<const VkPipelineCache> createPipelineCache(
        const std::shared_ptr<const VkDevice> &device, const std::vector<char> &initialData)
    {
        return createPipelineCache(
            device, static_cast<const void *>(initialData.data()), initialData.size());
    }
    std::vector<unsigned char> getPipelineCacheData(
        const std::shared_ptr<const VkDevice> &device,
        const std::shared_ptr<const VkPipelineCache> &pipelineCache,
        std::vector<unsigned char> bufferIn = {})
    {
        std::size_t dataSize = 0;
        handleVulkanResult(vkGetPipelineCacheData(*device, *pipelineCache, &dataSize, nullptr),
                           "vkGetPipelineCacheData");
        do
        {
            bufferIn.resize(dataSize);
            // handle size changes due to different threads
        } while(handleVulkanResult(
                    vkGetPipelineCacheData(
                        *device, *pipelineCache, &dataSize, static_cast<void *>(bufferIn.data())),
                    "vkGetPipelineCacheData") == VK_INCOMPLETE);
        bufferIn.resize(dataSize);
        return bufferIn;
    }
    std::shared_ptr<const VkFence> createFence(const std::shared_ptr<const VkDevice> &device,
                                               bool signaled = false)
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
        handleVulkanResult(vkCreateFence(*device, &createInfo, nullptr, &holder->subobject),
                           "vkCreateFence");
        holder->valid = true;
        return std::shared_ptr<const VkFence>(holder, &holder->subobject);
    }
    std::shared_ptr<const VkCommandPool> createCommandPool(
        const std::shared_ptr<const VkDevice> &device,
        bool transient,
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
                           "vkCreateCommandPool");
        holder->valid = true;
        return std::shared_ptr<const VkCommandPool>(holder, &holder->subobject);
    }
    std::shared_ptr<const VkCommandBuffer> allocateCommandBuffer(
        const std::shared_ptr<const VkDevice> &device,
        std::shared_ptr<const VkCommandPool> commandPool,
        VkCommandBufferLevel level)
    {
        auto holder = std::make_shared<CommandBufferHolder>(device, std::move(commandPool));
        VkCommandBufferAllocateInfo allocateInfo{};
        allocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocateInfo.commandPool = *holder->commandPool;
        allocateInfo.level = level;
        allocateInfo.commandBufferCount = 1;
        handleVulkanResult(vkAllocateCommandBuffers(*device, &allocateInfo, &holder->commandBuffer),
                           "vkAllocateCommandBuffers");
        holder->vkFreeCommandBuffers = vkFreeCommandBuffers;
        return std::shared_ptr<const VkCommandBuffer>(holder, &holder->commandBuffer);
    }
    std::shared_ptr<const VkImageView> createImageView(
        const std::shared_ptr<const VkDevice> &device,
        std::shared_ptr<const VkImage> image,
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
    std::shared_ptr<const VkShaderModule> createShaderModule(
        const std::shared_ptr<const VkDevice> &device,
        const void *code,
        std::size_t codeSizeInBytes)
    {
        const auto vkDestroyShaderModule = this->vkDestroyShaderModule;
        auto destroyFn = [vkDestroyShaderModule](const std::shared_ptr<const VkDevice> &device,
                                                 VkShaderModule shader)
        {
            vkDestroyShaderModule(*device, shader, nullptr);
        };
        auto holder = std::make_shared<DeviceSubobjectHolder<VkShaderModule, decltype(destroyFn)>>(
            device, std::move(destroyFn));
        VkShaderModuleCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        createInfo.codeSize = codeSizeInBytes;
        createInfo.pCode = reinterpret_cast<const std::uint32_t *>(code);
        handleVulkanResult(vkCreateShaderModule(*device, &createInfo, nullptr, &holder->subobject),
                           "vkCreateShaderModule");
        holder->valid = true;
        return std::shared_ptr<const VkShaderModule>(holder, &holder->subobject);
    }
    std::shared_ptr<const VkShaderModule> createShaderModule(
        const std::shared_ptr<const VkDevice> &device,
        const std::uint32_t *code,
        std::size_t codeSizeInWords)
    {
        return createShaderModule(
            device, static_cast<const void *>(code), codeSizeInWords * sizeof(std::uint32_t));
    }
    template <std::size_t N>
    std::shared_ptr<const VkShaderModule> createShaderModule(
        const std::shared_ptr<const VkDevice> &device, const std::uint32_t(&code)[N])
    {
        return createShaderModule(device, code, N);
    }
    std::shared_ptr<const VkShaderModule> createShaderModule(
        const std::shared_ptr<const VkDevice> &device, const std::vector<std::uint32_t> &code)
    {
        return createShaderModule(device, code.data(), code.size());
    }
    std::shared_ptr<const VkShaderModule> createShaderModule(
        const std::shared_ptr<const VkDevice> &device,
        const char *code,
        std::size_t codeSizeInBytes) // requires VK_NV_glsl_shader extension
    {
        return createShaderModule(device, static_cast<const void *>(code), codeSizeInBytes);
    }
    std::shared_ptr<const VkPipeline> createPipeline(
        const std::shared_ptr<const VkDevice> &device,
        std::shared_ptr<const VkShaderModule> vertexShader,
        std::shared_ptr<const VkShaderModule> fragmentShader,
        bool depthWritingEnabled,
        bool earlyZTestEnabled,
        std::shared_ptr<const VkPipelineLayout> layout,
        std::shared_ptr<const VkRenderPass> renderPass,
        std::uint32_t subpass,
        const std::shared_ptr<const VkPipelineCache> &pipelineCache = nullptr)
    {
        auto vkDestroyPipeline = this->vkDestroyPipeline;
        auto destroyFn =
            [vkDestroyPipeline](const std::shared_ptr<const VkDevice> &device, VkPipeline pipeline)
        {
            vkDestroyPipeline(*device, pipeline, nullptr);
        };
        typedef decltype(destroyFn) destroyFnType;
        struct Holder final
        {
            std::shared_ptr<const VkShaderModule> vertexShader;
            std::shared_ptr<const VkShaderModule> fragmentShader;
            std::shared_ptr<const VkPipelineLayout> layout;
            std::shared_ptr<const VkRenderPass> renderPass;
            DeviceSubobjectHolder<VkPipeline, destroyFnType> subobject;
            Holder(std::shared_ptr<const VkShaderModule> vertexShader,
                   std::shared_ptr<const VkShaderModule> fragmentShader,
                   std::shared_ptr<const VkPipelineLayout> layout,
                   std::shared_ptr<const VkRenderPass> renderPass,
                   std::shared_ptr<const VkDevice> device,
                   destroyFnType &&destroyFn)
                : vertexShader(std::move(vertexShader)),
                  fragmentShader(std::move(fragmentShader)),
                  layout(std::move(layout)),
                  renderPass(std::move(renderPass)),
                  subobject(std::move(device), std::move(destroyFn))
            {
            }
        };
        const std::size_t stageCount = 2;
        VkPipelineShaderStageCreateInfo stages[stageCount] = {};
        {
            VkPipelineShaderStageCreateInfo &vertexStage = stages[0];
            vertexStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
            vertexStage.stage = VK_SHADER_STAGE_VERTEX_BIT;
            vertexStage.module = *vertexShader;
            vertexStage.pName = "main";
        }
        {
            VkPipelineShaderStageCreateInfo &fragmentStage = stages[1];
            fragmentStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
            fragmentStage.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
            fragmentStage.module = *fragmentShader;
            fragmentStage.pName = "main";
        }
        const std::size_t vertexBindingDescriptionCount = 1;
        VkVertexInputBindingDescription vertexBindingDescriptions[vertexBindingDescriptionCount] =
            {};
        vertexBindingDescriptions[0].binding = 0;
        vertexBindingDescriptions[0].stride = sizeof(VulkanVertex);
        vertexBindingDescriptions[0].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
        const std::size_t vertexAttributeDescriptionCount = 3;
        VkVertexInputAttributeDescription
            vertexAttributeDescriptions[vertexAttributeDescriptionCount] = {};
        // must match VulkanVertex and vulkan.vert
        vertexAttributeDescriptions[0].location = VulkanVertex::positionLocation;
        vertexAttributeDescriptions[0].binding = 0;
        vertexAttributeDescriptions[0].format = VulkanVertex::positionFormat;
        vertexAttributeDescriptions[0].offset = offsetof(VulkanVertex, position);
        vertexAttributeDescriptions[1].location = VulkanVertex::colorLocation;
        vertexAttributeDescriptions[1].binding = 0;
        vertexAttributeDescriptions[1].format = VulkanVertex::colorFormat;
        vertexAttributeDescriptions[1].offset = offsetof(VulkanVertex, color);
        vertexAttributeDescriptions[2].location = VulkanVertex::textureCoordinatesLocation;
        vertexAttributeDescriptions[2].binding = 0;
        vertexAttributeDescriptions[2].format = VulkanVertex::textureCoordinatesFormat;
        vertexAttributeDescriptions[2].offset = offsetof(VulkanVertex, textureCoordinates);
        VkPipelineVertexInputStateCreateInfo vertexInputState{};
        vertexInputState.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        vertexInputState.vertexBindingDescriptionCount = vertexBindingDescriptionCount;
        vertexInputState.pVertexBindingDescriptions = &vertexBindingDescriptions[0];
        vertexInputState.vertexAttributeDescriptionCount = vertexAttributeDescriptionCount;
        vertexInputState.pVertexAttributeDescriptions = &vertexAttributeDescriptions[0];
        VkPipelineInputAssemblyStateCreateInfo inputAssemblyState{};
        inputAssemblyState.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        inputAssemblyState.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        inputAssemblyState.primitiveRestartEnable = VK_FALSE;
        VkPipelineViewportStateCreateInfo viewportState{};
        viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        viewportState.viewportCount = 1;
        viewportState.pViewports = nullptr;
        viewportState.scissorCount = 1;
        viewportState.pScissors = nullptr;
        VkPipelineRasterizationStateCreateInfo rasterizationState{};
        rasterizationState.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        rasterizationState.depthClampEnable = VK_FALSE;
        rasterizationState.rasterizerDiscardEnable = earlyZTestEnabled ? VK_TRUE : VK_FALSE;
        rasterizationState.polygonMode = VK_POLYGON_MODE_FILL;
        rasterizationState.cullMode = VK_CULL_MODE_BACK_BIT;
        rasterizationState.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
        rasterizationState.depthBiasEnable = VK_FALSE;
        rasterizationState.lineWidth = 1;
        VkPipelineMultisampleStateCreateInfo multisampleState{};
        multisampleState.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        multisampleState.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
        multisampleState.sampleShadingEnable = VK_FALSE;
        multisampleState.alphaToCoverageEnable = VK_FALSE;
        multisampleState.alphaToOneEnable = VK_FALSE;
        VkPipelineDepthStencilStateCreateInfo depthStencilState{};
        depthStencilState.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
        depthStencilState.depthTestEnable = VK_FALSE;
#warning todo: enable depth testing
        depthStencilState.depthWriteEnable = depthWritingEnabled ? VK_TRUE : VK_FALSE;
        depthStencilState.depthCompareOp = VK_COMPARE_OP_LESS;
        VkPipelineColorBlendAttachmentState colorBlendAttachmentState{};
        colorBlendAttachmentState.blendEnable = VK_FALSE;
#warning todo: enable blending
        colorBlendAttachmentState.colorWriteMask =
            VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT
            | VK_COLOR_COMPONENT_A_BIT;
        VkPipelineColorBlendStateCreateInfo colorBlendState{};
        colorBlendState.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        colorBlendState.logicOpEnable = VK_FALSE;
        colorBlendState.attachmentCount = 1;
        colorBlendState.pAttachments = &colorBlendAttachmentState;
        std::initializer_list<VkDynamicState> dynamicStates = {
            VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR,
        };
        VkPipelineDynamicStateCreateInfo dynamicState{};
        dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
        dynamicState.dynamicStateCount = dynamicStates.size();
        dynamicState.pDynamicStates = dynamicStates.begin();
        VkGraphicsPipelineCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        createInfo.stageCount = stageCount;
        createInfo.pStages = &stages[0];
        createInfo.pVertexInputState = &vertexInputState;
        createInfo.pInputAssemblyState = &inputAssemblyState;
        createInfo.pViewportState = &viewportState;
        createInfo.pRasterizationState = &rasterizationState;
        createInfo.pMultisampleState = &multisampleState;
        createInfo.pDepthStencilState = &depthStencilState;
        createInfo.pColorBlendState = &colorBlendState;
        createInfo.pDynamicState = &dynamicState;
        createInfo.layout = *layout;
        createInfo.renderPass = *renderPass;
        createInfo.subpass = subpass;
        createInfo.basePipelineHandle = VK_NULL_HANDLE;
        createInfo.basePipelineIndex = -1;
        auto holder = std::make_shared<Holder>(std::move(vertexShader),
                                               std::move(fragmentShader),
                                               std::move(layout),
                                               std::move(renderPass),
                                               device,
                                               std::move(destroyFn));
        handleVulkanResult(
            vkCreateGraphicsPipelines(*device,
                                      pipelineCache ? *pipelineCache : VK_NULL_HANDLE,
                                      1,
                                      &createInfo,
                                      nullptr,
                                      &holder->subobject.subobject),
            "vkCreateGraphicsPipelines");
        holder->subobject.valid = true;
        return std::shared_ptr<const VkPipeline>(holder, &holder->subobject.subobject);
    }
    std::shared_ptr<const VkPipelineLayout> createPipelineLayout(
        const std::shared_ptr<const VkDevice> &device,
        std::vector<std::shared_ptr<const VkDescriptorSetLayout>> descriptorSetLayouts,
        const std::vector<VkPushConstantRange> &pushConstantRanges)
    {
        const auto vkDestroyPipelineLayout = this->vkDestroyPipelineLayout;
        auto destroyFn = [vkDestroyPipelineLayout](const std::shared_ptr<const VkDevice> &device,
                                                   VkPipelineLayout pipelineLayout)
        {
            vkDestroyPipelineLayout(*device, pipelineLayout, nullptr);
        };
        typedef decltype(destroyFn) destroyFnType;
        struct Holder final
        {
            std::vector<std::shared_ptr<const VkDescriptorSetLayout>> descriptorSetLayouts;
            DeviceSubobjectHolder<VkPipelineLayout, destroyFnType> subobject;
            Holder(std::vector<std::shared_ptr<const VkDescriptorSetLayout>> descriptorSetLayouts,
                   std::shared_ptr<const VkDevice> device,
                   destroyFnType &&destroyFn)
                : descriptorSetLayouts(std::move(descriptorSetLayouts)),
                  subobject(std::move(device), std::move(destroyFn))
            {
            }
        };
        VkPipelineLayoutCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        std::vector<VkDescriptorSetLayout> descriptorSetLayoutArray;
        descriptorSetLayoutArray.reserve(descriptorSetLayouts.size());
        for(auto &i : descriptorSetLayouts)
            descriptorSetLayoutArray.push_back(*i);
        createInfo.setLayoutCount = descriptorSetLayoutArray.size();
        createInfo.pSetLayouts = descriptorSetLayoutArray.data();
        createInfo.pushConstantRangeCount = pushConstantRanges.size();
        createInfo.pPushConstantRanges = pushConstantRanges.data();
        auto holder =
            std::make_shared<Holder>(std::move(descriptorSetLayouts), device, std::move(destroyFn));
        handleVulkanResult(
            vkCreatePipelineLayout(*device, &createInfo, nullptr, &holder->subobject.subobject),
            "vkCreatePipelineLayout");
        holder->subobject.valid = true;
        return std::shared_ptr<const VkPipelineLayout>(holder, &holder->subobject.subobject);
    }
    std::shared_ptr<const VkRenderPass> createRenderPass(
        const std::shared_ptr<const VkDevice> &device, VkFormat swapChainImageFormat)
    {
        const auto vkDestroyRenderPass = this->vkDestroyRenderPass;
        auto destroyFn = [vkDestroyRenderPass](const std::shared_ptr<const VkDevice> &device,
                                               VkRenderPass renderPass)
        {
            vkDestroyRenderPass(*device, renderPass, nullptr);
        };
        typedef decltype(destroyFn) destroyFnType;
        struct Holder final
        {
            DeviceSubobjectHolder<VkRenderPass, destroyFnType> subobject;
            Holder(std::shared_ptr<const VkDevice> device, destroyFnType &&destroyFn)
                : subobject(std::move(device), std::move(destroyFn))
            {
            }
        };
        VkRenderPassCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        const std::size_t attachmentCount = 1;
        VkAttachmentDescription attachments[attachmentCount] = {};
        const std::size_t swapChainImageAttachmentIndex = 0;
        VkAttachmentDescription &swapChainImageDescription =
            attachments[swapChainImageAttachmentIndex];
        swapChainImageDescription.flags = 0;
        swapChainImageDescription.format = swapChainImageFormat;
        swapChainImageDescription.samples = VK_SAMPLE_COUNT_1_BIT;
        swapChainImageDescription.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        swapChainImageDescription.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        swapChainImageDescription.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        swapChainImageDescription.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        swapChainImageDescription.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        swapChainImageDescription.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
        createInfo.attachmentCount = attachmentCount;
        createInfo.pAttachments = &attachments[0];
        VkSubpassDescription subpass{};
        subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.inputAttachmentCount = 0;
        subpass.pInputAttachments = nullptr;
        const std::size_t subpassColorAttachmentCount = 1;
        VkAttachmentReference subpassColorAttachments[subpassColorAttachmentCount] = {};
        VkAttachmentReference &swapChainImageReference = subpassColorAttachments[0];
        swapChainImageReference.attachment = swapChainImageAttachmentIndex;
        swapChainImageReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        subpass.colorAttachmentCount = subpassColorAttachmentCount;
        subpass.pColorAttachments = &subpassColorAttachments[0];
        subpass.pResolveAttachments = nullptr;
        subpass.pDepthStencilAttachment = nullptr;
        subpass.preserveAttachmentCount = 0;
        subpass.pPreserveAttachments = nullptr;
        createInfo.subpassCount = 1;
        createInfo.pSubpasses = &subpass;
        std::initializer_list<VkSubpassDependency> dependencies = {};
        createInfo.dependencyCount = dependencies.size();
        createInfo.pDependencies = dependencies.begin();
        auto holder = std::make_shared<Holder>(device, std::move(destroyFn));
        handleVulkanResult(
            vkCreateRenderPass(*device, &createInfo, nullptr, &holder->subobject.subobject),
            "vkCreateRenderPass");
        holder->subobject.valid = true;
        return std::shared_ptr<const VkRenderPass>(holder, &holder->subobject.subobject);
    }
    std::shared_ptr<const VkFramebuffer> createFramebuffer(
        const std::shared_ptr<const VkDevice> &device,
        std::shared_ptr<const VkRenderPass> renderPass,
        std::vector<std::shared_ptr<const VkImageView>> attachments,
        std::uint32_t width,
        std::uint32_t height,
        std::uint32_t layers = 1)
    {
        const auto vkDestroyFramebuffer = this->vkDestroyFramebuffer;
        auto destroyFn = [vkDestroyFramebuffer](const std::shared_ptr<const VkDevice> &device,
                                                VkFramebuffer framebuffer)
        {
            vkDestroyFramebuffer(*device, framebuffer, nullptr);
        };
        typedef decltype(destroyFn) destroyFnType;
        struct Holder final
        {
            std::shared_ptr<const VkRenderPass> renderPass;
            std::vector<std::shared_ptr<const VkImageView>> attachments;
            DeviceSubobjectHolder<VkFramebuffer, destroyFnType> subobject;
            Holder(std::shared_ptr<const VkRenderPass> renderPass,
                   std::vector<std::shared_ptr<const VkImageView>> attachments,
                   std::shared_ptr<const VkDevice> device,
                   destroyFnType &&destroyFn)
                : renderPass(std::move(renderPass)),
                  attachments(std::move(attachments)),
                  subobject(std::move(device), std::move(destroyFn))
            {
            }
        };
        VkFramebufferCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        createInfo.renderPass = *renderPass;
        std::vector<VkImageView> attachmentArray;
        attachmentArray.reserve(attachments.size());
        for(auto &i : attachments)
            attachmentArray.push_back(*i);
        createInfo.attachmentCount = attachmentArray.size();
        createInfo.pAttachments = attachmentArray.data();
        createInfo.width = width;
        createInfo.height = height;
        createInfo.layers = layers;
        auto holder = std::make_shared<Holder>(
            std::move(renderPass), std::move(attachments), device, std::move(destroyFn));
        handleVulkanResult(
            vkCreateFramebuffer(*device, &createInfo, nullptr, &holder->subobject.subobject),
            "vkCreateFramebuffer");
        holder->subobject.valid = true;
        return std::shared_ptr<const VkFramebuffer>(holder, &holder->subobject.subobject);
    }
    std::shared_ptr<const VkDeviceMemory> allocateDeviceMemory(
        const std::shared_ptr<const VkDevice> &device,
        VkDeviceSize allocationSize,
        std::uint32_t memoryTypeIndex)
    {
        const auto vkFreeMemory = this->vkFreeMemory;
        auto destroyFn = [vkFreeMemory](const std::shared_ptr<const VkDevice> &device,
                                        VkDeviceMemory deviceMemory)
        {
            vkFreeMemory(*device, deviceMemory, nullptr);
        };
        auto holder = std::make_shared<DeviceSubobjectHolder<VkDeviceMemory, decltype(destroyFn)>>(
            device, std::move(destroyFn));
        VkMemoryAllocateInfo allocateInfo{};
        allocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocateInfo.allocationSize = allocationSize;
        allocateInfo.memoryTypeIndex = memoryTypeIndex;
        handleVulkanResult(vkAllocateMemory(*device, &allocateInfo, nullptr, &holder->subobject),
                           "vkAllocateMemory");
        holder->valid = true;
        return std::shared_ptr<const VkDeviceMemory>(holder, &holder->subobject);
    }
    std::shared_ptr<void *const> mapDeviceMemory(const std::shared_ptr<const VkDevice> &device,
                                                 std::shared_ptr<const VkDeviceMemory> deviceMemory)
    {
        struct Holder final
        {
            Holder(const Holder &) = delete;
            Holder &operator=(const Holder &) = delete;
            std::shared_ptr<const VkDeviceMemory> deviceMemory;
            std::shared_ptr<const VkDevice> device;
            void *mappedMemory;
            PFN_vkUnmapMemory vkUnmapMemory;
            explicit Holder(std::shared_ptr<const VkDeviceMemory> deviceMemory,
                            std::shared_ptr<const VkDevice> device,
                            PFN_vkMapMemory vkMapMemory,
                            PFN_vkUnmapMemory vkUnmapMemory)
                : deviceMemory(std::move(deviceMemory)),
                  device(std::move(device)),
                  mappedMemory(nullptr),
                  vkUnmapMemory(vkUnmapMemory)
            {
                handleVulkanResult(
                    vkMapMemory(
                        *this->device, *this->deviceMemory, 0, VK_WHOLE_SIZE, 0, &mappedMemory),
                    "vkMapMemory");
            }
            ~Holder()
            {
                vkUnmapMemory(*device, *deviceMemory);
            }
        };
        auto holder =
            std::make_shared<Holder>(std::move(deviceMemory), device, vkMapMemory, vkUnmapMemory);
        return std::shared_ptr<void *const>(holder, &holder->mappedMemory);
    }
    void createNewPipeline(const std::shared_ptr<const VkDevice> &device, VkFormat surfaceFormat)
    {
        if(renderPass && renderPipeline && renderPipelineLayout
           && renderPipelineFormat == surfaceFormat)
            return;
        renderPipelineFormat = surfaceFormat;
        renderPass = createRenderPass(device, surfaceFormat);
        VkPushConstantRange pushConstantRange{};
        pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
        pushConstantRange.offset = 0;
        pushConstantRange.size = sizeof(PushConstants);
        renderPipelineLayout = createPipelineLayout(device, {}, {pushConstantRange});
        renderPipeline = createPipeline(device,
                                        vertexShaderModule,
                                        fragmentShaderModule,
                                        true,
                                        false,
                                        renderPipelineLayout,
                                        renderPass,
                                        0,
                                        pipelineCache);
    }
    void createNewSwapchain(const std::shared_ptr<const VkDevice> &device) // destroys old swapchain
    {
        handleVulkanResult(vkDeviceWaitIdle(*device), "vkDeviceWaitIdle");
        frames.clear();
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
        VkPresentModeKHR selectedPresentMode = VK_PRESENT_MODE_FIFO_KHR;
        std::string presentModeStr = "fifo";
        for(auto presentMode : presentModes)
        {
            switch(presentMode)
            {
            case VK_PRESENT_MODE_IMMEDIATE_KHR:
                if(selectedPresentMode != VK_PRESENT_MODE_MAILBOX_KHR)
                {
                    presentModeStr = "immediate";
                    selectedPresentMode = VK_PRESENT_MODE_IMMEDIATE_KHR;
                }
                break;
            case VK_PRESENT_MODE_MAILBOX_KHR:
                presentModeStr = "mailbox";
                selectedPresentMode = VK_PRESENT_MODE_MAILBOX_KHR;
                break;
            case VK_PRESENT_MODE_FIFO_KHR:
                break;
            case VK_PRESENT_MODE_FIFO_RELAXED_KHR:
                break;

            // so the compiler doesn't complain about missing enum values
            case VK_PRESENT_MODE_RANGE_SIZE_KHR:
            case VK_PRESENT_MODE_MAX_ENUM_KHR:
                break;
            }
        }
        logging::log(logging::Level::Info, "VulkanDriver", "Present Mode: " + presentModeStr);
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
        swapchainExtent = surfaceCapabilities.currentExtent;
        if(swapchainExtent.width == static_cast<std::uint32_t>(-1)) // special extent
        {
            int w, h;
            SDL_GL_GetDrawableSize(vulkanDriver->getWindow(), &w, &h);
            swapchainExtent.width = w;
            swapchainExtent.height = h;
        }
        if(swapchainExtent.width == 0 || swapchainExtent.height == 0)
            return;
        {
            std::ostringstream ss;
            ss << "swapchainExtent = <" << swapchainExtent.width << ", " << swapchainExtent.height
               << ">";
            logging::log(logging::Level::Info, "VulkanDriver", ss.str());
        }
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
        swapchainCreateInfo.presentMode = selectedPresentMode;
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
            handleVulkanResult(
                vkGetSwapchainImagesKHR(
                    *device, *swapchain, &swapchainImageCount, holder->images.data()),
                "vkGetSwapchainImagesKHR");
            for(auto &image : holder->images)
                swapchainImages.push_back(std::shared_ptr<const VkImage>(holder, &image));

            for(std::uint32_t frameIndex = 0; frameIndex < swapchainImages.size(); frameIndex++)
            {
                swapchainImageViews.push_back(createImageView(device,
                                                              swapchainImages[frameIndex],
                                                              VK_IMAGE_VIEW_TYPE_2D,
                                                              surfaceFormat.format,
                                                              VK_IMAGE_ASPECT_COLOR_BIT,
                                                              0,
                                                              1,
                                                              0,
                                                              1));
            }
            createNewPipeline(device, surfaceFormat.format);
        }
        catch(...)
        {
            swapchain = nullptr;
            throw;
        }
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
            vkGetPhysicalDeviceMemoryProperties(i, &physicalDeviceMemoryProperties);
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
        std::shared_ptr<const VkDevice> device;
        {
            auto deviceHolder = std::make_shared<DeviceHolder>(instance);
            handleVulkanResult(
                vkCreateDevice(*physicalDevice, &deviceCreateInfo, nullptr, &deviceHolder->device),
                "vkCreateDevice");
            deviceHolder->loadDestroyFunction(this);
            device = std::shared_ptr<const VkDevice>(deviceHolder, &deviceHolder->device);
            atomicDevice.set(device);
        }
        loadDeviceFunctions(device);
        graphicsQueue = getQueue(device, graphicsQueueFamilyIndex, 0);
        if(graphicsQueueFamilyIndex == presentQueueFamilyIndex)
            presentQueue = graphicsQueue;
        else
            presentQueue = getQueue(device, presentQueueFamilyIndex, 0);
        vertexShaderModule = createShaderModule(device, vulkanVertexShader);
        fragmentShaderModule = createShaderModule(device, vulkanFragmentShader);
        std::shared_ptr<const VkShaderModule> fragmentShaderModule = nullptr;
        imageAvailableSemaphore = createSemaphore(device);
        renderingFinishedSemaphore = createSemaphore(device);
        pipelineCache = createPipelineCache(device);
        createNewSwapchain(device);
    }
    void destroyGraphicsContext() noexcept
    {
#warning finish
        const std::shared_ptr<const VkDevice> device = atomicDevice.get();
        vkDeviceWaitIdle(*device); // ignore return value
        frames.clear();
        swapchainImages.clear();
        swapchainImageViews.clear();
        swapchain = nullptr;
        renderPipeline = nullptr;
        renderPipelineLayout = nullptr;
        renderPass = nullptr;
        pipelineCache = nullptr;
        imageAvailableSemaphore = nullptr;
        renderingFinishedSemaphore = nullptr;
        vertexShaderModule = nullptr;
        fragmentShaderModule = nullptr;
        graphicsQueue = nullptr;
        presentQueue = nullptr;
        atomicDevice.set(nullptr);
        vertexBufferMemoryManager.shrink();
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
    void renderFrame(VulkanCommandBuffer &commandBufferIn)
    {
        constexprAssert(commandBufferIn.finished);
        const std::shared_ptr<const VkDevice> device = atomicDevice.get();
        if(!swapchain)
            return;
        std::uint32_t imageIndex = 0;
        VkResult acquireNextImageResult = vkAcquireNextImageKHR(
            *device, *swapchain, UINT64_MAX, *imageAvailableSemaphore, VK_NULL_HANDLE, &imageIndex);
        if(acquireNextImageResult == VK_ERROR_OUT_OF_DATE_KHR)
        {
            createNewSwapchain(device);
            return;
        }
        handleVulkanResult(acquireNextImageResult, "vkAcquireNextImageKHR");
        if(frames.size() >= 6)
        {
            FrameObjects frame = std::move(frames.front());
            frames.pop_front();
            frames.push_back(std::move(frame));
        }
        else
        {
            frames.emplace_back(createFence(device, true));
        }
        FrameObjects &frame = frames.back();
        handleVulkanResult(
            vkWaitForFences(*device, 1, &*frame.fence, VK_TRUE, static_cast<std::uint64_t>(-1)),
            "vkWaitForFences");
        frame.resetObjects();
        handleVulkanResult(vkResetFences(*device, 1, &*frame.fence), "vkResetFences");
        auto commandBuffer =
            allocateCommandBuffer(device,
                                  createCommandPool(device, true, false, graphicsQueueFamilyIndex),
                                  VK_COMMAND_BUFFER_LEVEL_PRIMARY);
        VkCommandBufferBeginInfo commandBufferBeginInfo{};
        commandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        commandBufferBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        handleVulkanResult(vkBeginCommandBuffer(*commandBuffer, &commandBufferBeginInfo),
                           "vkBeginCommandBuffer");
#warning finish
        VkViewport viewport;
        viewport.x = 0;
        viewport.y = 0;
        viewport.width = swapchainExtent.width;
        viewport.height = swapchainExtent.height;
        viewport.minDepth = 0;
        viewport.maxDepth = 1;
        vkCmdSetViewport(*commandBuffer, 0, 1, &viewport);
        VkRenderPassBeginInfo renderPassBeginInfo{};
        renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassBeginInfo.renderPass = *renderPass;
        frame.addObject(renderPass);
        auto framebuffer = createFramebuffer(device,
                                             renderPass,
                                             {swapchainImageViews[imageIndex]},
                                             swapchainExtent.width,
                                             swapchainExtent.height);
        renderPassBeginInfo.framebuffer = *framebuffer;
        frame.addObject(framebuffer);
        renderPassBeginInfo.renderArea.offset.x = 0;
        renderPassBeginInfo.renderArea.offset.y = 0;
        renderPassBeginInfo.renderArea.extent = swapchainExtent;
        const std::size_t clearValueCount = 1;
        VkClearValue clearValues[clearValueCount];
        clearValues[0].color.float32[0] = commandBufferIn.clearColor.red;
        clearValues[0].color.float32[1] = commandBufferIn.clearColor.green;
        clearValues[0].color.float32[2] = commandBufferIn.clearColor.blue;
        clearValues[0].color.float32[3] = commandBufferIn.clearColor.opacity;
        renderPassBeginInfo.clearValueCount = clearValueCount;
        renderPassBeginInfo.pClearValues = &clearValues[0];
        vkCmdSetScissor(*commandBuffer, 0, 1, &renderPassBeginInfo.renderArea);
        vkCmdBindPipeline(*commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, *renderPipeline);
        vkCmdBeginRenderPass(*commandBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
        frame.addObject(renderPipeline);
        MemoryRenderBuffer renderBuffer;
        Texture texture(TextureId{});
        shape::renderCube(renderBuffer,
                          RenderLayer::Opaque,
                          {texture, texture, texture, texture, texture, texture},
                          {true, true, true, true, true, true},
                          util::Vector3F(-0.5f),
                          util::Vector3F(0.5f),
                          {rgbF(0, 0, 0),
                           rgbF(0, 0, 1),
                           rgbF(0, 1, 0),
                           rgbF(0, 1, 1),
                           rgbF(1, 0, 0),
                           rgbF(1, 0, 1),
                           rgbF(1, 1, 0),
                           rgbF(1, 1, 1)});
        auto &triangles = renderBuffer.getTriangles(RenderLayer::Opaque);
        std::vector<VulkanTriangle> vulkanTriangles;
        vulkanTriangles.reserve(triangles.size());
        for(auto &triangle : triangles)
        {
            vulkanTriangles.push_back(triangle);
        }
        double currentTime = std::chrono::duration_cast<std::chrono::duration<double>>(
                                 std::chrono::steady_clock::now().time_since_epoch()).count();
        VertexBufferAllocation vertexBuffer =
            vertexBufferMemoryManager.allocate(sizeof(VulkanTriangle) * vulkanTriangles.size());
        std::memcpy(
            static_cast<void *>(reinterpret_cast<char *>(*(*vertexBuffer.getBase())->mapMemory())
                                + vertexBuffer.getOffset()),
            static_cast<const void *>(vulkanTriangles.data()),
            sizeof(VulkanTriangle) * vulkanTriangles.size());
        VkBuffer vertexBufferHandle = *(*vertexBuffer.getBase())->getBuffer();
        VkDeviceSize vertexBufferOffset = vertexBuffer.getOffset();
        vkCmdBindVertexBuffers(*commandBuffer, 0, 1, &vertexBufferHandle, &vertexBufferOffset);
        frame.addObject(vertexBuffer);
        PushConstants pushConstants{};
        auto outputSize = vulkanDriver->getOutputSize();
        float scaleX = std::get<0>(outputSize);
        float scaleY = std::get<1>(outputSize);
        scaleX /= std::get<1>(outputSize);
        scaleY /= std::get<0>(outputSize);
        if(scaleX < 1)
            scaleX = 1;
        if(scaleY < 1)
            scaleY = 1;
        const float nearPlane = 0.01;
        const float farPlane = 100;
        pushConstants.transformMatrix = util::Matrix4x4F::rotateX(currentTime)
                                            .concat(util::Matrix4x4F::rotateY(currentTime * 1.234))
                                            .concat(util::Matrix4x4F::translate(0, 1, -3))
                                            .concat(util::Matrix4x4F::frustum(-nearPlane * scaleX,
                                                                              nearPlane * scaleX,
                                                                              -nearPlane * scaleY,
                                                                              nearPlane * scaleY,
                                                                              nearPlane,
                                                                              farPlane))
                                            .concat(util::Matrix4x4F::scale(1, -1, 1));
        vkCmdPushConstants(*commandBuffer,
                           *renderPipelineLayout,
                           VK_SHADER_STAGE_VERTEX_BIT,
                           0,
                           sizeof(PushConstants),
                           static_cast<const void *>(&pushConstants));
        frame.addObject(renderPipelineLayout);
        vkCmdDraw(*commandBuffer, 3 * vulkanTriangles.size(), 1, 0, 0);
        vkCmdEndRenderPass(*commandBuffer);
        handleVulkanResult(vkEndCommandBuffer(*commandBuffer), "vkEndCommandBuffer");
        const VkPipelineStageFlags waitDestinationStageFlags = VK_PIPELINE_STAGE_TRANSFER_BIT;
        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.waitSemaphoreCount = 1;
        submitInfo.pWaitSemaphores = &*imageAvailableSemaphore;
        submitInfo.pWaitDstStageMask = &waitDestinationStageFlags;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &*commandBuffer;
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.pSignalSemaphores = &*renderingFinishedSemaphore;
        frame.addObject(std::move(commandBuffer));
        handleVulkanResult(vkQueueSubmit(*graphicsQueue, 1, &submitInfo, *frame.fence),
                           "vkQueueSubmit");
        VkPresentInfoKHR presentInfo{};
        presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        presentInfo.waitSemaphoreCount = 1;
        presentInfo.pWaitSemaphores = &*renderingFinishedSemaphore;
        presentInfo.swapchainCount = 1;
        presentInfo.pSwapchains = &*swapchain;
        presentInfo.pImageIndices = &imageIndex;
        auto presentResult = vkQueuePresentKHR(*presentQueue, &presentInfo);
        if(presentResult == VK_ERROR_OUT_OF_DATE_KHR || presentResult == VK_SUBOPTIMAL_KHR)
        {
            createNewSwapchain(device);
            return;
        }
        handleVulkanResult(presentResult, "vkQueuePresentKHR");
        auto currentWindowSize = vulkanDriver->getOutputSize();
        if(swapchainExtent.width != std::get<0>(currentWindowSize)
           || swapchainExtent.height != std::get<1>(currentWindowSize))
        {
            createNewSwapchain(device);
            return;
        }
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
    auto retval = std::make_shared<Implementation::VulkanRenderBuffer>(maximumSizes);
    retval->attachDevice(implementation->atomicDevice.get(), implementation);
    return retval;
}

std::shared_ptr<CommandBuffer> VulkanDriver::makeCommandBuffer()
{
#warning finish
    return std::make_shared<Implementation::VulkanCommandBuffer>(implementation->atomicDevice.get(),
                                                                 implementation);
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
