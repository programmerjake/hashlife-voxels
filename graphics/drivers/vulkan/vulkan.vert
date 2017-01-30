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
#version 450
#extension GL_ARB_separate_shader_objects : enable

/* input variables; must match
 * VulkanDriver::Implementation::VulkanVertex
 * in vulkan_driver.cpp
 */
layout(location = 0) in vec3 positionIn;
layout(location = 1) in vec4 colorIn;
layout(location = 2) in vec2 textureCoordinatesIn;

layout(push_constant) uniform PushConstants
{
    /* push constant variables; must match
     * VulkanDriver::Implementation::PushConstants
     * in vulkan_driver.cpp
     */
    mat4 transformMatrix;
} pushConstants;

/* output variables; must match input variables in vulkan.frag */
layout(location = 0) out vec4 colorOut;
layout(location = 1) out vec2 textureCoordinatesOut;

out gl_PerVertex
{
    vec4 gl_Position;
};

void main()
{
    colorOut = colorIn;
    textureCoordinatesOut = textureCoordinatesIn;
    gl_Position = pushConstants.transformMatrix * vec4(positionIn, 1);
}