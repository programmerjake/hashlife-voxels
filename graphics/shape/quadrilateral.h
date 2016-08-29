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

#ifndef GRAPHICS_SHAPE_QUADRILATERAL_H_
#define GRAPHICS_SHAPE_QUADRILATERAL_H_

#include "../render.h"
#include "../texture.h"
#include "../triangle.h"
#include "../color.h"
#include "../../util/vector.h"

namespace programmerjake
{
namespace voxels
{
namespace graphics
{
namespace shape
{
inline void renderQuadrilateral(RenderBuffer &renderBuffer,
                                RenderLayer renderLayer,
                                Texture texture,
                                util::Vector3F p0,
                                ColorI c0,
                                util::Vector3F p1,
                                ColorI c1,
                                util::Vector3F p2,
                                ColorI c2,
                                util::Vector3F p3,
                                ColorI c3)
{
    auto normal = getTriangleNormalOrZero(p0, p1, p2);
    Vertex v0(p0, {texture.nunv.u, texture.nunv.v}, c0, normal);
    Vertex v1(p1, {texture.pupv.u, texture.nunv.v}, c1, normal);
    Vertex v2(p2, {texture.pupv.u, texture.pupv.v}, c2, normal);
    Vertex v3(p3, {texture.nunv.u, texture.pupv.v}, c3, normal);
    renderBuffer.appendTriangles(
        renderLayer, {{v0, v1, v2, texture.textureId}, {v2, v3, v0, texture.textureId}});
}

inline void renderQuadrilateral(RenderBuffer &renderBuffer,
                                RenderLayer renderLayer,
                                Texture texture,
                                util::Vector3F p0,
                                ColorI c0,
                                util::Vector3F p1,
                                ColorI c1,
                                util::Vector3F p2,
                                ColorI c2,
                                util::Vector3F p3,
                                ColorI c3,
                                const Transform &tform)
{
    auto normal = transformNormal(tform, getTriangleNormalOrZero(p0, p1, p2));
    Vertex v0(transform(tform, p0), {texture.nunv.u, texture.nunv.v}, c0, normal);
    Vertex v1(transform(tform, p1), {texture.pupv.u, texture.nunv.v}, c1, normal);
    Vertex v2(transform(tform, p2), {texture.pupv.u, texture.pupv.v}, c2, normal);
    Vertex v3(transform(tform, p3), {texture.nunv.u, texture.pupv.v}, c3, normal);
    renderBuffer.appendTriangles(
        renderLayer, {{v0, v1, v2, texture.textureId}, {v2, v3, v0, texture.textureId}});
}
}
}
}
}
#endif /* GRAPHICS_SHAPE_QUADRILATERAL_H_ */
