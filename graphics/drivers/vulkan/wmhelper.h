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

#ifndef GRAPHICS_DRIVERS_VULKAN_WMHELPER_H_
#define GRAPHICS_DRIVERS_VULKAN_WMHELPER_H_

#include "../sdl2_driver.h"
#include "SDL_syswm.h"
#include "vulkan_system_headers.h"
#include "vulkan_instance.h"

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
struct WMHelper
{
    virtual ~WMHelper() = default;
    SDL_SYSWM_TYPE syswmType;
    explicit WMHelper(SDL_SYSWM_TYPE syswmType) : syswmType(syswmType)
    {
    }
    virtual const char *getWMExtensionName() const noexcept = 0;
    virtual std::shared_ptr<const VkSurfaceKHR> createSurface(
        std::shared_ptr<const VulkanInstance> vulkanInstance,
        const SDL_SysWMinfo &wmInfo) const = 0;
};
class WMHelpers final
{
private:
    const WMHelper *const *front;
    const WMHelper *const *back;

public:
    WMHelpers() noexcept;
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
}
}
}
}
}

#endif /* GRAPHICS_DRIVERS_VULKAN_WMHELPER_H_ */
