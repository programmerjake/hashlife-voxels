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

#ifndef GRAPHICS_DRIVERS_VULKAN_VULKAN_OBJECT_H_
#define GRAPHICS_DRIVERS_VULKAN_VULKAN_OBJECT_H_

#include <tuple>
#include <memory>
#include <utility>

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
template <typename Handle, typename DestroyFunction, typename BaseHandle, typename... OtherHandles>
struct VulkanObject final
{
    VulkanObject(const VulkanObject &) = delete;
    VulkanObject &operator=(const VulkanObject &) = delete;
    std::tuple<std::shared_ptr<const BaseHandle>, std::shared_ptr<const OtherHandles>...> handles;
    Handle handle{};
    DestroyFunction destroyFunction;
    bool needsDestroy = false;
    VulkanObject(DestroyFunction destroyFunction,
                 std::shared_ptr<const BaseHandle> baseHandle,
                 std::shared_ptr<const OtherHandles>... otherHandles)
        : handles(std::move(baseHandle), std::move(otherHandles)...),
          destroyFunction(std::move(destroyFunction))
    {
    }
    ~VulkanObject()
    {
        if(needsDestroy)
            destroyFunction(handle, *std::get<0>(handles));
    }
};

template <typename Handle, typename DestroyFunction, typename... OtherHandles>
struct VulkanObject<Handle, DestroyFunction, void, OtherHandles...> final
{
    VulkanObject(const VulkanObject &) = delete;
    VulkanObject &operator=(const VulkanObject &) = delete;
    std::tuple<std::shared_ptr<const OtherHandles>...> handles;
    Handle handle{};
    DestroyFunction destroyFunction;
    bool needsDestroy = false;
    VulkanObject(DestroyFunction destroyFunction,
                 std::shared_ptr<const OtherHandles>... otherHandles)
        : handles(std::move(otherHandles)...), destroyFunction(std::move(destroyFunction))
    {
    }
    ~VulkanObject()
    {
        if(needsDestroy)
            destroyFunction(handle);
    }
};
}
}
}
}
}

#endif /* GRAPHICS_DRIVERS_VULKAN_VULKAN_OBJECT_H_ */
