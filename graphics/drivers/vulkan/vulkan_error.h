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

#ifndef GRAPHICS_DRIVERS_VULKAN_VULKAN_ERROR_H_
#define GRAPHICS_DRIVERS_VULKAN_VULKAN_ERROR_H_

#include "vulkan_system_headers.h"
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
std::string getVulkanResultString(VkResult result);
void throwVulkanResultMessage(VkResult result, const char *functionName);
inline VkResult handleVulkanResult(VkResult result,
                                   const char *functionName,
                                   bool mustBeSuccess = false,
                                   bool allowOutOfDate = false)
{
    if((result < 0 && (!allowOutOfDate || result != VK_ERROR_OUT_OF_DATE_KHR))
       || (mustBeSuccess && result != VK_SUCCESS))
        throwVulkanResultMessage(result, functionName);
    return result;
}
}
}
}
}
}

#endif /* GRAPHICS_DRIVERS_VULKAN_VULKAN_ERROR_H_ */
