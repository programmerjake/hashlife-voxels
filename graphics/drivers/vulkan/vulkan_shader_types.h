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

#ifndef GRAPHICS_DRIVERS_VULKAN_VULKAN_SHADER_TYPES_H_
#define GRAPHICS_DRIVERS_VULKAN_VULKAN_SHADER_TYPES_H_

#include "../../color.h"
#include "../../texture_coordinates.h"
#include "../../../util/vector.h"
#include "../../../util/matrix.h"

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
    constexpr VulkanVertex(const Vertex &v) noexcept : position(v.getPosition()),
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
    constexpr VulkanTriangle(const Triangle &v) noexcept
        : v{VulkanVertex(v.vertices[0]), VulkanVertex(v.vertices[1]), VulkanVertex(v.vertices[2])}
    {
    }
    constexpr VulkanTriangle(const TriangleWithoutNormal &v) noexcept
        : v{VulkanVertex(v.vertices[0]), VulkanVertex(v.vertices[1]), VulkanVertex(v.vertices[2])}
    {
    }
};

static_assert(sizeof(VulkanTriangle) == 3 * sizeof(VulkanVertex), "");

struct PushConstants final
{
    // must match PushConstants in vulkan.vert
    VulkanMat4 transformMatrix;
};

static_assert(sizeof(PushConstants) <= 128, "PushConstants is bigger than minimum guaranteed size");
}
}
}
}
}

#endif /* GRAPHICS_DRIVERS_VULKAN_VULKAN_SHADER_TYPES_H_ */
