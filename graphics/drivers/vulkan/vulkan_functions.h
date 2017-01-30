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

#ifndef GRAPHICS_DRIVERS_VULKAN_VULKAN_FUNCTIONS_H_
#define GRAPHICS_DRIVERS_VULKAN_VULKAN_FUNCTIONS_H_

#include "vulkan_system_headers.h"
#include <cassert>

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
#define VULKAN_DRIVER_END_FUNCTION()
#define VULKAN_DRIVER_FUNCTIONS()                                                 \
    VULKAN_DRIVER_DEVICE_FUNCTION(AcquireNextImageKHR)                            \
    VULKAN_DRIVER_DEVICE_FUNCTION(AllocateCommandBuffers)                         \
    VULKAN_DRIVER_DEVICE_FUNCTION(AllocateDescriptorSets)                         \
    VULKAN_DRIVER_DEVICE_FUNCTION(AllocateMemory)                                 \
    VULKAN_DRIVER_DEVICE_FUNCTION(BeginCommandBuffer)                             \
    VULKAN_DRIVER_DEVICE_FUNCTION(BindBufferMemory)                               \
    VULKAN_DRIVER_DEVICE_FUNCTION(BindImageMemory)                                \
    VULKAN_DRIVER_DEVICE_FUNCTION(CmdBeginQuery)                                  \
    VULKAN_DRIVER_DEVICE_FUNCTION(CmdBeginRenderPass)                             \
    VULKAN_DRIVER_DEVICE_FUNCTION(CmdBindDescriptorSets)                          \
    VULKAN_DRIVER_DEVICE_FUNCTION(CmdBindIndexBuffer)                             \
    VULKAN_DRIVER_DEVICE_FUNCTION(CmdBindPipeline)                                \
    VULKAN_DRIVER_DEVICE_FUNCTION(CmdBindVertexBuffers)                           \
    VULKAN_DRIVER_DEVICE_FUNCTION(CmdBlitImage)                                   \
    VULKAN_DRIVER_DEVICE_FUNCTION(CmdClearAttachments)                            \
    VULKAN_DRIVER_DEVICE_FUNCTION(CmdClearColorImage)                             \
    VULKAN_DRIVER_DEVICE_FUNCTION(CmdClearDepthStencilImage)                      \
    VULKAN_DRIVER_DEVICE_FUNCTION(CmdCopyBuffer)                                  \
    VULKAN_DRIVER_DEVICE_FUNCTION(CmdCopyBufferToImage)                           \
    VULKAN_DRIVER_DEVICE_FUNCTION(CmdCopyImage)                                   \
    VULKAN_DRIVER_DEVICE_FUNCTION(CmdCopyImageToBuffer)                           \
    VULKAN_DRIVER_DEVICE_FUNCTION(CmdCopyQueryPoolResults)                        \
    VULKAN_DRIVER_DEVICE_FUNCTION(CmdDispatch)                                    \
    VULKAN_DRIVER_DEVICE_FUNCTION(CmdDispatchIndirect)                            \
    VULKAN_DRIVER_DEVICE_FUNCTION(CmdDraw)                                        \
    VULKAN_DRIVER_DEVICE_FUNCTION(CmdDrawIndexed)                                 \
    VULKAN_DRIVER_DEVICE_FUNCTION(CmdDrawIndexedIndirect)                         \
    VULKAN_DRIVER_DEVICE_FUNCTION(CmdDrawIndirect)                                \
    VULKAN_DRIVER_DEVICE_FUNCTION(CmdEndQuery)                                    \
    VULKAN_DRIVER_DEVICE_FUNCTION(CmdEndRenderPass)                               \
    VULKAN_DRIVER_DEVICE_FUNCTION(CmdExecuteCommands)                             \
    VULKAN_DRIVER_DEVICE_FUNCTION(CmdFillBuffer)                                  \
    VULKAN_DRIVER_DEVICE_FUNCTION(CmdNextSubpass)                                 \
    VULKAN_DRIVER_DEVICE_FUNCTION(CmdPipelineBarrier)                             \
    VULKAN_DRIVER_DEVICE_FUNCTION(CmdPushConstants)                               \
    VULKAN_DRIVER_DEVICE_FUNCTION(CmdResetEvent)                                  \
    VULKAN_DRIVER_DEVICE_FUNCTION(CmdResetQueryPool)                              \
    VULKAN_DRIVER_DEVICE_FUNCTION(CmdResolveImage)                                \
    VULKAN_DRIVER_DEVICE_FUNCTION(CmdSetBlendConstants)                           \
    VULKAN_DRIVER_DEVICE_FUNCTION(CmdSetDepthBias)                                \
    VULKAN_DRIVER_DEVICE_FUNCTION(CmdSetDepthBounds)                              \
    VULKAN_DRIVER_DEVICE_FUNCTION(CmdSetEvent)                                    \
    VULKAN_DRIVER_DEVICE_FUNCTION(CmdSetLineWidth)                                \
    VULKAN_DRIVER_DEVICE_FUNCTION(CmdSetScissor)                                  \
    VULKAN_DRIVER_DEVICE_FUNCTION(CmdSetStencilCompareMask)                       \
    VULKAN_DRIVER_DEVICE_FUNCTION(CmdSetStencilReference)                         \
    VULKAN_DRIVER_DEVICE_FUNCTION(CmdSetStencilWriteMask)                         \
    VULKAN_DRIVER_DEVICE_FUNCTION(CmdSetViewport)                                 \
    VULKAN_DRIVER_DEVICE_FUNCTION(CmdUpdateBuffer)                                \
    VULKAN_DRIVER_DEVICE_FUNCTION(CmdWaitEvents)                                  \
    VULKAN_DRIVER_DEVICE_FUNCTION(CmdWriteTimestamp)                              \
    VULKAN_DRIVER_DEVICE_FUNCTION(CreateBuffer)                                   \
    VULKAN_DRIVER_DEVICE_FUNCTION(CreateBufferView)                               \
    VULKAN_DRIVER_DEVICE_FUNCTION(CreateCommandPool)                              \
    VULKAN_DRIVER_DEVICE_FUNCTION(CreateComputePipelines)                         \
    VULKAN_DRIVER_DEVICE_FUNCTION(CreateDescriptorPool)                           \
    VULKAN_DRIVER_DEVICE_FUNCTION(CreateDescriptorSetLayout)                      \
    VULKAN_DRIVER_DEVICE_FUNCTION(CreateEvent)                                    \
    VULKAN_DRIVER_DEVICE_FUNCTION(CreateFence)                                    \
    VULKAN_DRIVER_DEVICE_FUNCTION(CreateFramebuffer)                              \
    VULKAN_DRIVER_DEVICE_FUNCTION(CreateGraphicsPipelines)                        \
    VULKAN_DRIVER_DEVICE_FUNCTION(CreateImage)                                    \
    VULKAN_DRIVER_DEVICE_FUNCTION(CreateImageView)                                \
    VULKAN_DRIVER_DEVICE_FUNCTION(CreatePipelineCache)                            \
    VULKAN_DRIVER_DEVICE_FUNCTION(CreatePipelineLayout)                           \
    VULKAN_DRIVER_DEVICE_FUNCTION(CreateQueryPool)                                \
    VULKAN_DRIVER_DEVICE_FUNCTION(CreateRenderPass)                               \
    VULKAN_DRIVER_DEVICE_FUNCTION(CreateSampler)                                  \
    VULKAN_DRIVER_DEVICE_FUNCTION(CreateSemaphore)                                \
    VULKAN_DRIVER_DEVICE_FUNCTION(CreateShaderModule)                             \
    VULKAN_DRIVER_DEVICE_FUNCTION(CreateSwapchainKHR)                             \
    VULKAN_DRIVER_DEVICE_FUNCTION(DestroyBuffer)                                  \
    VULKAN_DRIVER_DEVICE_FUNCTION(DestroyBufferView)                              \
    VULKAN_DRIVER_DEVICE_FUNCTION(DestroyCommandPool)                             \
    VULKAN_DRIVER_DEVICE_FUNCTION(DestroyDescriptorPool)                          \
    VULKAN_DRIVER_DEVICE_FUNCTION(DestroyDescriptorSetLayout)                     \
    VULKAN_DRIVER_DEVICE_FUNCTION(DestroyDevice)                                  \
    VULKAN_DRIVER_DEVICE_FUNCTION(DestroyEvent)                                   \
    VULKAN_DRIVER_DEVICE_FUNCTION(DestroyFence)                                   \
    VULKAN_DRIVER_DEVICE_FUNCTION(DestroyFramebuffer)                             \
    VULKAN_DRIVER_DEVICE_FUNCTION(DestroyImage)                                   \
    VULKAN_DRIVER_DEVICE_FUNCTION(DestroyImageView)                               \
    VULKAN_DRIVER_DEVICE_FUNCTION(DestroyInstance)                                \
    VULKAN_DRIVER_DEVICE_FUNCTION(DestroyPipeline)                                \
    VULKAN_DRIVER_DEVICE_FUNCTION(DestroyPipelineCache)                           \
    VULKAN_DRIVER_DEVICE_FUNCTION(DestroyPipelineLayout)                          \
    VULKAN_DRIVER_DEVICE_FUNCTION(DestroyQueryPool)                               \
    VULKAN_DRIVER_DEVICE_FUNCTION(DestroyRenderPass)                              \
    VULKAN_DRIVER_DEVICE_FUNCTION(DestroySampler)                                 \
    VULKAN_DRIVER_DEVICE_FUNCTION(DestroySemaphore)                               \
    VULKAN_DRIVER_DEVICE_FUNCTION(DestroyShaderModule)                            \
    VULKAN_DRIVER_DEVICE_FUNCTION(DestroySwapchainKHR)                            \
    VULKAN_DRIVER_DEVICE_FUNCTION(DeviceWaitIdle)                                 \
    VULKAN_DRIVER_DEVICE_FUNCTION(EndCommandBuffer)                               \
    VULKAN_DRIVER_DEVICE_FUNCTION(FlushMappedMemoryRanges)                        \
    VULKAN_DRIVER_DEVICE_FUNCTION(FreeCommandBuffers)                             \
    VULKAN_DRIVER_DEVICE_FUNCTION(FreeDescriptorSets)                             \
    VULKAN_DRIVER_DEVICE_FUNCTION(FreeMemory)                                     \
    VULKAN_DRIVER_DEVICE_FUNCTION(GetBufferMemoryRequirements)                    \
    VULKAN_DRIVER_DEVICE_FUNCTION(GetDeviceMemoryCommitment)                      \
    VULKAN_DRIVER_DEVICE_FUNCTION(GetDeviceQueue)                                 \
    VULKAN_DRIVER_DEVICE_FUNCTION(GetEventStatus)                                 \
    VULKAN_DRIVER_DEVICE_FUNCTION(GetFenceStatus)                                 \
    VULKAN_DRIVER_DEVICE_FUNCTION(GetImageMemoryRequirements)                     \
    VULKAN_DRIVER_DEVICE_FUNCTION(GetImageSparseMemoryRequirements)               \
    VULKAN_DRIVER_DEVICE_FUNCTION(GetImageSubresourceLayout)                      \
    VULKAN_DRIVER_DEVICE_FUNCTION(GetPipelineCacheData)                           \
    VULKAN_DRIVER_DEVICE_FUNCTION(GetQueryPoolResults)                            \
    VULKAN_DRIVER_DEVICE_FUNCTION(GetRenderAreaGranularity)                       \
    VULKAN_DRIVER_DEVICE_FUNCTION(GetSwapchainImagesKHR)                          \
    VULKAN_DRIVER_DEVICE_FUNCTION(InvalidateMappedMemoryRanges)                   \
    VULKAN_DRIVER_DEVICE_FUNCTION(MapMemory)                                      \
    VULKAN_DRIVER_DEVICE_FUNCTION(MergePipelineCaches)                            \
    VULKAN_DRIVER_DEVICE_FUNCTION(QueueBindSparse)                                \
    VULKAN_DRIVER_DEVICE_FUNCTION(QueuePresentKHR)                                \
    VULKAN_DRIVER_DEVICE_FUNCTION(QueueSubmit)                                    \
    VULKAN_DRIVER_DEVICE_FUNCTION(QueueWaitIdle)                                  \
    VULKAN_DRIVER_DEVICE_FUNCTION(ResetCommandBuffer)                             \
    VULKAN_DRIVER_DEVICE_FUNCTION(ResetCommandPool)                               \
    VULKAN_DRIVER_DEVICE_FUNCTION(ResetDescriptorPool)                            \
    VULKAN_DRIVER_DEVICE_FUNCTION(ResetEvent)                                     \
    VULKAN_DRIVER_DEVICE_FUNCTION(ResetFences)                                    \
    VULKAN_DRIVER_DEVICE_FUNCTION(SetEvent)                                       \
    VULKAN_DRIVER_DEVICE_FUNCTION(UnmapMemory)                                    \
    VULKAN_DRIVER_DEVICE_FUNCTION(UpdateDescriptorSets)                           \
    VULKAN_DRIVER_DEVICE_FUNCTION(WaitForFences)                                  \
    VULKAN_DRIVER_GLOBAL_FUNCTION(CreateInstance)                                 \
    VULKAN_DRIVER_GLOBAL_FUNCTION(EnumerateInstanceExtensionProperties)           \
    VULKAN_DRIVER_GLOBAL_FUNCTION(EnumerateInstanceLayerProperties)               \
    VULKAN_DRIVER_INSTANCE_FUNCTION(CreateDevice)                                 \
    VULKAN_DRIVER_INSTANCE_FUNCTION(DestroySurfaceKHR)                            \
    VULKAN_DRIVER_INSTANCE_FUNCTION(EnumerateDeviceExtensionProperties)           \
    VULKAN_DRIVER_INSTANCE_FUNCTION(EnumerateDeviceLayerProperties)               \
    VULKAN_DRIVER_INSTANCE_FUNCTION(EnumeratePhysicalDevices)                     \
    VULKAN_DRIVER_INSTANCE_FUNCTION(GetDeviceProcAddr)                            \
    VULKAN_DRIVER_INSTANCE_FUNCTION(GetPhysicalDeviceFeatures)                    \
    VULKAN_DRIVER_INSTANCE_FUNCTION(GetPhysicalDeviceFormatProperties)            \
    VULKAN_DRIVER_INSTANCE_FUNCTION(GetPhysicalDeviceImageFormatProperties)       \
    VULKAN_DRIVER_INSTANCE_FUNCTION(GetPhysicalDeviceMemoryProperties)            \
    VULKAN_DRIVER_INSTANCE_FUNCTION(GetPhysicalDeviceProperties)                  \
    VULKAN_DRIVER_INSTANCE_FUNCTION(GetPhysicalDeviceQueueFamilyProperties)       \
    VULKAN_DRIVER_INSTANCE_FUNCTION(GetPhysicalDeviceSparseImageFormatProperties) \
    VULKAN_DRIVER_INSTANCE_FUNCTION(GetPhysicalDeviceSurfaceCapabilitiesKHR)      \
    VULKAN_DRIVER_INSTANCE_FUNCTION(GetPhysicalDeviceSurfaceFormatsKHR)           \
    VULKAN_DRIVER_INSTANCE_FUNCTION(GetPhysicalDeviceSurfacePresentModesKHR)      \
    VULKAN_DRIVER_INSTANCE_FUNCTION(GetPhysicalDeviceSurfaceSupportKHR)           \
    VULKAN_DRIVER_END_FUNCTION()
struct VulkanFunctions final
{
private:
    struct PrivateAccess final
    {
        friend struct VulkanFunctions;

    private:
        PrivateAccess() = default;
    };
    static void handleFunctionLoadError(const char *name);

public:
    std::shared_ptr<void> vulkanLibrary;
    PFN_vkGetInstanceProcAddr GetInstanceProcAddr;
    VulkanFunctions(std::shared_ptr<void> vulkanLibraryIn, PrivateAccess);
    static std::shared_ptr<VulkanFunctions> loadVulkanLibrary();
#define VULKAN_DRIVER_GLOBAL_FUNCTION(name) PFN_vk##name name = nullptr;
#define VULKAN_DRIVER_INSTANCE_FUNCTION(name) PFN_vk##name name = nullptr;
#define VULKAN_DRIVER_DEVICE_FUNCTION(name) PFN_vk##name name = nullptr;
    VULKAN_DRIVER_FUNCTIONS()
#undef VULKAN_DRIVER_GLOBAL_FUNCTION
#undef VULKAN_DRIVER_INSTANCE_FUNCTION
#undef VULKAN_DRIVER_DEVICE_FUNCTION
    template <typename Fn>
    void loadGlobalFunction(Fn &fn, const char *name) const
    {
        fn = reinterpret_cast<Fn>(GetInstanceProcAddr(VK_NULL_HANDLE, name));
        if(!fn)
            handleFunctionLoadError(name);
    }
    template <typename Fn>
    void loadInstanceFunction(Fn &fn, VkInstance instance, const char *name) const
    {
        fn = reinterpret_cast<Fn>(GetInstanceProcAddr(instance, name));
        if(!fn)
            handleFunctionLoadError(name);
    }
    template <typename Fn>
    void loadDeviceFunction(Fn &fn, VkDevice device, const char *name) const
    {
        fn = reinterpret_cast<Fn>(GetDeviceProcAddr(device, name));
        if(!fn)
            handleFunctionLoadError(name);
    }

private:
    void loadGlobalFunctions()
    {
#define VULKAN_DRIVER_GLOBAL_FUNCTION(name) loadGlobalFunction(name, "vk" #name);
#define VULKAN_DRIVER_INSTANCE_FUNCTION(name)
#define VULKAN_DRIVER_DEVICE_FUNCTION(name)
        VULKAN_DRIVER_FUNCTIONS()
#undef VULKAN_DRIVER_GLOBAL_FUNCTION
#undef VULKAN_DRIVER_INSTANCE_FUNCTION
#undef VULKAN_DRIVER_DEVICE_FUNCTION
    }

public:
    void loadInstanceFunctions(VkInstance instance)
    {
        assert(instance != VK_NULL_HANDLE);
#define VULKAN_DRIVER_GLOBAL_FUNCTION(name)
#define VULKAN_DRIVER_INSTANCE_FUNCTION(name) loadInstanceFunction(name, instance, "vk" #name);
#define VULKAN_DRIVER_DEVICE_FUNCTION(name)
        VULKAN_DRIVER_FUNCTIONS()
#undef VULKAN_DRIVER_GLOBAL_FUNCTION
#undef VULKAN_DRIVER_INSTANCE_FUNCTION
#undef VULKAN_DRIVER_DEVICE_FUNCTION
    }
    void loadDeviceFunctions(VkDevice device)
    {
        assert(device != VK_NULL_HANDLE);
#define VULKAN_DRIVER_GLOBAL_FUNCTION(name)
#define VULKAN_DRIVER_INSTANCE_FUNCTION(name)
#define VULKAN_DRIVER_DEVICE_FUNCTION(name) loadDeviceFunction(name, device, "vk" #name);
        VULKAN_DRIVER_FUNCTIONS()
#undef VULKAN_DRIVER_GLOBAL_FUNCTION
#undef VULKAN_DRIVER_INSTANCE_FUNCTION
#undef VULKAN_DRIVER_DEVICE_FUNCTION
    }
#undef VULKAN_DRIVER_FUNCTIONS
#undef VULKAN_DRIVER_END_FUNCTION
};
}
}
}
}
}

#endif /* GRAPHICS_DRIVERS_VULKAN_VULKAN_FUNCTIONS_H_ */
