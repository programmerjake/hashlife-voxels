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

#ifndef GRAPHICS_SHAPE_CUBE_H_
#define GRAPHICS_SHAPE_CUBE_H_

#include "../render.h"
#include "../texture.h"
#include "../triangle.h"
#include "../color.h"
#include "quadrilateral.h"
#include "../../block/block.h"
#include "../../util/enum.h"
#include "../../util/constexpr_assert.h"
#include "../../util/constexpr_array.h"

namespace programmerjake
{
namespace voxels
{
namespace graphics
{
namespace shape
{
inline void renderCubeFaceNX(RenderBuffer &renderBuffer,
                             RenderLayer renderLayer,
                             Texture texture,
                             util::Vector3F nxnynz,
                             util::Vector3F pxpypz,
                             const util::array<util::array<util::array<ColorF, 2>, 2>, 2> &colors)
{
    const auto p0 = util::Vector3F(nxnynz.x, nxnynz.y, nxnynz.z);
    const auto c0 = colors[0][0][0];
    const auto p2 = util::Vector3F(nxnynz.x, pxpypz.y, nxnynz.z);
    const auto c2 = colors[0][1][0];
    const auto p4 = util::Vector3F(nxnynz.x, nxnynz.y, pxpypz.z);
    const auto c4 = colors[0][0][1];
    const auto p6 = util::Vector3F(nxnynz.x, pxpypz.y, pxpypz.z);
    const auto c6 = colors[0][1][1];
    renderQuadrilateral(renderBuffer, renderLayer, texture, p0, c0, p4, c4, p6, c6, p2, c2);
}

inline void renderCubeFaceNX(RenderBuffer &renderBuffer,
                             RenderLayer renderLayer,
                             Texture texture,
                             util::Vector3F nxnynz,
                             util::Vector3F pxpypz,
                             const util::array<util::array<util::array<ColorF, 2>, 2>, 2> &colors,
                             const Transform &tform)
{
    const auto p0 = util::Vector3F(nxnynz.x, nxnynz.y, nxnynz.z);
    const auto c0 = colors[0][0][0];
    const auto p2 = util::Vector3F(nxnynz.x, pxpypz.y, nxnynz.z);
    const auto c2 = colors[0][1][0];
    const auto p4 = util::Vector3F(nxnynz.x, nxnynz.y, pxpypz.z);
    const auto c4 = colors[0][0][1];
    const auto p6 = util::Vector3F(nxnynz.x, pxpypz.y, pxpypz.z);
    const auto c6 = colors[0][1][1];
    renderQuadrilateral(renderBuffer, renderLayer, texture, p0, c0, p4, c4, p6, c6, p2, c2, tform);
}

inline void renderCubeFacePX(RenderBuffer &renderBuffer,
                             RenderLayer renderLayer,
                             Texture texture,
                             util::Vector3F nxnynz,
                             util::Vector3F pxpypz,
                             const util::array<util::array<util::array<ColorF, 2>, 2>, 2> &colors)
{
    const auto p1 = util::Vector3F(pxpypz.x, nxnynz.y, nxnynz.z);
    const auto c1 = colors[1][0][0];
    const auto p3 = util::Vector3F(pxpypz.x, pxpypz.y, nxnynz.z);
    const auto c3 = colors[1][1][0];
    const auto p5 = util::Vector3F(pxpypz.x, nxnynz.y, pxpypz.z);
    const auto c5 = colors[1][0][1];
    const auto p7 = util::Vector3F(pxpypz.x, pxpypz.y, pxpypz.z);
    const auto c7 = colors[1][1][1];
    renderQuadrilateral(renderBuffer, renderLayer, texture, p5, c5, p1, c1, p3, c3, p7, c7);
}

inline void renderCubeFacePX(RenderBuffer &renderBuffer,
                             RenderLayer renderLayer,
                             Texture texture,
                             util::Vector3F nxnynz,
                             util::Vector3F pxpypz,
                             const util::array<util::array<util::array<ColorF, 2>, 2>, 2> &colors,
                             const Transform &tform)
{
    const auto p1 = util::Vector3F(pxpypz.x, nxnynz.y, nxnynz.z);
    const auto c1 = colors[1][0][0];
    const auto p3 = util::Vector3F(pxpypz.x, pxpypz.y, nxnynz.z);
    const auto c3 = colors[1][1][0];
    const auto p5 = util::Vector3F(pxpypz.x, nxnynz.y, pxpypz.z);
    const auto c5 = colors[1][0][1];
    const auto p7 = util::Vector3F(pxpypz.x, pxpypz.y, pxpypz.z);
    const auto c7 = colors[1][1][1];
    renderQuadrilateral(renderBuffer, renderLayer, texture, p5, c5, p1, c1, p3, c3, p7, c7, tform);
}

inline void renderCubeFaceNY(RenderBuffer &renderBuffer,
                             RenderLayer renderLayer,
                             Texture texture,
                             util::Vector3F nxnynz,
                             util::Vector3F pxpypz,
                             const util::array<util::array<util::array<ColorF, 2>, 2>, 2> &colors)
{
    const auto p0 = util::Vector3F(nxnynz.x, nxnynz.y, nxnynz.z);
    const auto c0 = colors[0][0][0];
    const auto p1 = util::Vector3F(pxpypz.x, nxnynz.y, nxnynz.z);
    const auto c1 = colors[1][0][0];
    const auto p4 = util::Vector3F(nxnynz.x, nxnynz.y, pxpypz.z);
    const auto c4 = colors[0][0][1];
    const auto p5 = util::Vector3F(pxpypz.x, nxnynz.y, pxpypz.z);
    const auto c5 = colors[1][0][1];
    renderQuadrilateral(renderBuffer, renderLayer, texture, p0, c0, p1, c1, p5, c5, p4, c4);
}

inline void renderCubeFaceNY(RenderBuffer &renderBuffer,
                             RenderLayer renderLayer,
                             Texture texture,
                             util::Vector3F nxnynz,
                             util::Vector3F pxpypz,
                             const util::array<util::array<util::array<ColorF, 2>, 2>, 2> &colors,
                             const Transform &tform)
{
    const auto p0 = util::Vector3F(nxnynz.x, nxnynz.y, nxnynz.z);
    const auto c0 = colors[0][0][0];
    const auto p1 = util::Vector3F(pxpypz.x, nxnynz.y, nxnynz.z);
    const auto c1 = colors[1][0][0];
    const auto p4 = util::Vector3F(nxnynz.x, nxnynz.y, pxpypz.z);
    const auto c4 = colors[0][0][1];
    const auto p5 = util::Vector3F(pxpypz.x, nxnynz.y, pxpypz.z);
    const auto c5 = colors[1][0][1];
    renderQuadrilateral(renderBuffer, renderLayer, texture, p0, c0, p1, c1, p5, c5, p4, c4, tform);
}

inline void renderCubeFacePY(RenderBuffer &renderBuffer,
                             RenderLayer renderLayer,
                             Texture texture,
                             util::Vector3F nxnynz,
                             util::Vector3F pxpypz,
                             const util::array<util::array<util::array<ColorF, 2>, 2>, 2> &colors)
{
    const auto p2 = util::Vector3F(nxnynz.x, pxpypz.y, nxnynz.z);
    const auto c2 = colors[0][1][0];
    const auto p3 = util::Vector3F(pxpypz.x, pxpypz.y, nxnynz.z);
    const auto c3 = colors[1][1][0];
    const auto p6 = util::Vector3F(nxnynz.x, pxpypz.y, pxpypz.z);
    const auto c6 = colors[0][1][1];
    const auto p7 = util::Vector3F(pxpypz.x, pxpypz.y, pxpypz.z);
    const auto c7 = colors[1][1][1];
    renderQuadrilateral(renderBuffer, renderLayer, texture, p6, c6, p7, c7, p3, c3, p2, c2);
}

inline void renderCubeFacePY(RenderBuffer &renderBuffer,
                             RenderLayer renderLayer,
                             Texture texture,
                             util::Vector3F nxnynz,
                             util::Vector3F pxpypz,
                             const util::array<util::array<util::array<ColorF, 2>, 2>, 2> &colors,
                             const Transform &tform)
{
    const auto p2 = util::Vector3F(nxnynz.x, pxpypz.y, nxnynz.z);
    const auto c2 = colors[0][1][0];
    const auto p3 = util::Vector3F(pxpypz.x, pxpypz.y, nxnynz.z);
    const auto c3 = colors[1][1][0];
    const auto p6 = util::Vector3F(nxnynz.x, pxpypz.y, pxpypz.z);
    const auto c6 = colors[0][1][1];
    const auto p7 = util::Vector3F(pxpypz.x, pxpypz.y, pxpypz.z);
    const auto c7 = colors[1][1][1];
    renderQuadrilateral(renderBuffer, renderLayer, texture, p6, c6, p7, c7, p3, c3, p2, c2, tform);
}

inline void renderCubeFaceNZ(RenderBuffer &renderBuffer,
                             RenderLayer renderLayer,
                             Texture texture,
                             util::Vector3F nxnynz,
                             util::Vector3F pxpypz,
                             const util::array<util::array<util::array<ColorF, 2>, 2>, 2> &colors)
{
    const auto p0 = util::Vector3F(nxnynz.x, nxnynz.y, nxnynz.z);
    const auto c0 = colors[0][0][0];
    const auto p1 = util::Vector3F(pxpypz.x, nxnynz.y, nxnynz.z);
    const auto c1 = colors[1][0][0];
    const auto p2 = util::Vector3F(nxnynz.x, pxpypz.y, nxnynz.z);
    const auto c2 = colors[0][1][0];
    const auto p3 = util::Vector3F(pxpypz.x, pxpypz.y, nxnynz.z);
    const auto c3 = colors[1][1][0];
    renderQuadrilateral(renderBuffer, renderLayer, texture, p1, c1, p0, c0, p2, c2, p3, c3);
}

inline void renderCubeFaceNZ(RenderBuffer &renderBuffer,
                             RenderLayer renderLayer,
                             Texture texture,
                             util::Vector3F nxnynz,
                             util::Vector3F pxpypz,
                             const util::array<util::array<util::array<ColorF, 2>, 2>, 2> &colors,
                             const Transform &tform)
{
    const auto p0 = util::Vector3F(nxnynz.x, nxnynz.y, nxnynz.z);
    const auto c0 = colors[0][0][0];
    const auto p1 = util::Vector3F(pxpypz.x, nxnynz.y, nxnynz.z);
    const auto c1 = colors[1][0][0];
    const auto p2 = util::Vector3F(nxnynz.x, pxpypz.y, nxnynz.z);
    const auto c2 = colors[0][1][0];
    const auto p3 = util::Vector3F(pxpypz.x, pxpypz.y, nxnynz.z);
    const auto c3 = colors[1][1][0];
    renderQuadrilateral(renderBuffer, renderLayer, texture, p1, c1, p0, c0, p2, c2, p3, c3, tform);
}

inline void renderCubeFacePZ(RenderBuffer &renderBuffer,
                             RenderLayer renderLayer,
                             Texture texture,
                             util::Vector3F nxnynz,
                             util::Vector3F pxpypz,
                             const util::array<util::array<util::array<ColorF, 2>, 2>, 2> &colors)
{
    const auto p4 = util::Vector3F(nxnynz.x, nxnynz.y, pxpypz.z);
    const auto c4 = colors[0][0][1];
    const auto p5 = util::Vector3F(pxpypz.x, nxnynz.y, pxpypz.z);
    const auto c5 = colors[1][0][1];
    const auto p6 = util::Vector3F(nxnynz.x, pxpypz.y, pxpypz.z);
    const auto c6 = colors[0][1][1];
    const auto p7 = util::Vector3F(pxpypz.x, pxpypz.y, pxpypz.z);
    const auto c7 = colors[1][1][1];
    renderQuadrilateral(renderBuffer, renderLayer, texture, p4, c4, p5, c5, p7, c7, p6, c6);
}

inline void renderCubeFacePZ(RenderBuffer &renderBuffer,
                             RenderLayer renderLayer,
                             Texture texture,
                             util::Vector3F nxnynz,
                             util::Vector3F pxpypz,
                             const util::array<util::array<util::array<ColorF, 2>, 2>, 2> &colors,
                             const Transform &tform)
{
    const auto p4 = util::Vector3F(nxnynz.x, nxnynz.y, pxpypz.z);
    const auto c4 = colors[0][0][1];
    const auto p5 = util::Vector3F(pxpypz.x, nxnynz.y, pxpypz.z);
    const auto c5 = colors[1][0][1];
    const auto p6 = util::Vector3F(nxnynz.x, pxpypz.y, pxpypz.z);
    const auto c6 = colors[0][1][1];
    const auto p7 = util::Vector3F(pxpypz.x, pxpypz.y, pxpypz.z);
    const auto c7 = colors[1][1][1];
    renderQuadrilateral(renderBuffer, renderLayer, texture, p4, c4, p5, c5, p7, c7, p6, c6, tform);
}

inline void renderCubeFace(RenderBuffer &renderBuffer,
                           RenderLayer renderLayer,
                           block::BlockFace face,
                           util::EnumArray<Texture, block::BlockFace> textures,
                           util::Vector3F nxnynz,
                           util::Vector3F pxpypz,
                           const util::array<util::array<util::array<ColorF, 2>, 2>, 2> &colors)
{
    switch(face)
    {
    case block::BlockFace::NX:
        renderCubeFaceNX(
            renderBuffer, renderLayer, textures[block::BlockFace::NX], nxnynz, pxpypz, colors);
        return;
    case block::BlockFace::PX:
        renderCubeFacePX(
            renderBuffer, renderLayer, textures[block::BlockFace::PX], nxnynz, pxpypz, colors);
        return;
    case block::BlockFace::NY:
        renderCubeFaceNY(
            renderBuffer, renderLayer, textures[block::BlockFace::NY], nxnynz, pxpypz, colors);
        return;
    case block::BlockFace::PY:
        renderCubeFacePY(
            renderBuffer, renderLayer, textures[block::BlockFace::PY], nxnynz, pxpypz, colors);
        return;
    case block::BlockFace::NZ:
        renderCubeFaceNZ(
            renderBuffer, renderLayer, textures[block::BlockFace::NZ], nxnynz, pxpypz, colors);
        return;
    case block::BlockFace::PZ:
        renderCubeFacePZ(
            renderBuffer, renderLayer, textures[block::BlockFace::PZ], nxnynz, pxpypz, colors);
        return;
    }
    constexprAssert(false);
}

inline void renderCubeFace(RenderBuffer &renderBuffer,
                           RenderLayer renderLayer,
                           block::BlockFace face,
                           util::EnumArray<Texture, block::BlockFace> textures,
                           util::Vector3F nxnynz,
                           util::Vector3F pxpypz,
                           const util::array<util::array<util::array<ColorF, 2>, 2>, 2> &colors,
                           const Transform &tform)
{
    switch(face)
    {
    case block::BlockFace::NX:
        renderCubeFaceNX(renderBuffer,
                         renderLayer,
                         textures[block::BlockFace::NX],
                         nxnynz,
                         pxpypz,
                         colors,
                         tform);
        return;
    case block::BlockFace::PX:
        renderCubeFacePX(renderBuffer,
                         renderLayer,
                         textures[block::BlockFace::PX],
                         nxnynz,
                         pxpypz,
                         colors,
                         tform);
        return;
    case block::BlockFace::NY:
        renderCubeFaceNY(renderBuffer,
                         renderLayer,
                         textures[block::BlockFace::NY],
                         nxnynz,
                         pxpypz,
                         colors,
                         tform);
        return;
    case block::BlockFace::PY:
        renderCubeFacePY(renderBuffer,
                         renderLayer,
                         textures[block::BlockFace::PY],
                         nxnynz,
                         pxpypz,
                         colors,
                         tform);
        return;
    case block::BlockFace::NZ:
        renderCubeFaceNZ(renderBuffer,
                         renderLayer,
                         textures[block::BlockFace::NZ],
                         nxnynz,
                         pxpypz,
                         colors,
                         tform);
        return;
    case block::BlockFace::PZ:
        renderCubeFacePZ(renderBuffer,
                         renderLayer,
                         textures[block::BlockFace::PZ],
                         nxnynz,
                         pxpypz,
                         colors,
                         tform);
        return;
    }
    constexprAssert(false);
}

inline void renderCube(RenderBuffer &renderBuffer,
                       RenderLayer renderLayer,
                       util::EnumArray<Texture, block::BlockFace> textures,
                       util::EnumArray<bool, block::BlockFace> faceFlags,
                       util::Vector3F nxnynz,
                       util::Vector3F pxpypz,
                       const util::array<util::array<util::array<ColorF, 2>, 2>, 2> &colors)
{
    if(faceFlags[block::BlockFace::NX])
        renderCubeFaceNX(
            renderBuffer, renderLayer, textures[block::BlockFace::NX], nxnynz, pxpypz, colors);
    if(faceFlags[block::BlockFace::PX])
        renderCubeFacePX(
            renderBuffer, renderLayer, textures[block::BlockFace::PX], nxnynz, pxpypz, colors);
    if(faceFlags[block::BlockFace::NY])
        renderCubeFaceNY(
            renderBuffer, renderLayer, textures[block::BlockFace::NY], nxnynz, pxpypz, colors);
    if(faceFlags[block::BlockFace::PY])
        renderCubeFacePY(
            renderBuffer, renderLayer, textures[block::BlockFace::PY], nxnynz, pxpypz, colors);
    if(faceFlags[block::BlockFace::NZ])
        renderCubeFaceNZ(
            renderBuffer, renderLayer, textures[block::BlockFace::NZ], nxnynz, pxpypz, colors);
    if(faceFlags[block::BlockFace::PZ])
        renderCubeFacePZ(
            renderBuffer, renderLayer, textures[block::BlockFace::PZ], nxnynz, pxpypz, colors);
}

inline void renderCube(RenderBuffer &renderBuffer,
                       RenderLayer renderLayer,
                       util::EnumArray<Texture, block::BlockFace> textures,
                       util::EnumArray<bool, block::BlockFace> faceFlags,
                       util::Vector3F nxnynz,
                       util::Vector3F pxpypz,
                       const util::array<util::array<util::array<ColorF, 2>, 2>, 2> &colors,
                       const Transform &tform)
{
    if(faceFlags[block::BlockFace::NX])
        renderCubeFaceNX(renderBuffer,
                         renderLayer,
                         textures[block::BlockFace::NX],
                         nxnynz,
                         pxpypz,
                         colors,
                         tform);
    if(faceFlags[block::BlockFace::PX])
        renderCubeFacePX(renderBuffer,
                         renderLayer,
                         textures[block::BlockFace::PX],
                         nxnynz,
                         pxpypz,
                         colors,
                         tform);
    if(faceFlags[block::BlockFace::NY])
        renderCubeFaceNY(renderBuffer,
                         renderLayer,
                         textures[block::BlockFace::NY],
                         nxnynz,
                         pxpypz,
                         colors,
                         tform);
    if(faceFlags[block::BlockFace::PY])
        renderCubeFacePY(renderBuffer,
                         renderLayer,
                         textures[block::BlockFace::PY],
                         nxnynz,
                         pxpypz,
                         colors,
                         tform);
    if(faceFlags[block::BlockFace::NZ])
        renderCubeFaceNZ(renderBuffer,
                         renderLayer,
                         textures[block::BlockFace::NZ],
                         nxnynz,
                         pxpypz,
                         colors,
                         tform);
    if(faceFlags[block::BlockFace::PZ])
        renderCubeFacePZ(renderBuffer,
                         renderLayer,
                         textures[block::BlockFace::PZ],
                         nxnynz,
                         pxpypz,
                         colors,
                         tform);
}

inline void renderCubeFaceNX(RenderBuffer &renderBuffer,
                             RenderLayer renderLayer,
                             Texture texture,
                             util::Vector3F nxnynz = util::Vector3F(0),
                             util::Vector3F pxpypz = util::Vector3F(1),
                             ColorF color = colorizeIdentityF())
{
    renderCubeFaceNX(renderBuffer,
                     renderLayer,
                     texture,
                     nxnynz,
                     pxpypz,
                     {color, color, color, color, color, color, color, color});
}

inline void renderCubeFaceNX(RenderBuffer &renderBuffer,
                             RenderLayer renderLayer,
                             Texture texture,
                             util::Vector3F nxnynz,
                             util::Vector3F pxpypz,
                             ColorF color,
                             const Transform &tform)
{
    renderCubeFaceNX(renderBuffer,
                     renderLayer,
                     texture,
                     nxnynz,
                     pxpypz,
                     {color, color, color, color, color, color, color, color},
                     tform);
}

inline void renderCubeFacePX(RenderBuffer &renderBuffer,
                             RenderLayer renderLayer,
                             Texture texture,
                             util::Vector3F nxnynz = util::Vector3F(0),
                             util::Vector3F pxpypz = util::Vector3F(1),
                             ColorF color = colorizeIdentityF())
{
    renderCubeFacePX(renderBuffer,
                     renderLayer,
                     texture,
                     nxnynz,
                     pxpypz,
                     {color, color, color, color, color, color, color, color});
}

inline void renderCubeFacePX(RenderBuffer &renderBuffer,
                             RenderLayer renderLayer,
                             Texture texture,
                             util::Vector3F nxnynz,
                             util::Vector3F pxpypz,
                             ColorF color,
                             const Transform &tform)
{
    renderCubeFacePX(renderBuffer,
                     renderLayer,
                     texture,
                     nxnynz,
                     pxpypz,
                     {color, color, color, color, color, color, color, color},
                     tform);
}

inline void renderCubeFaceNY(RenderBuffer &renderBuffer,
                             RenderLayer renderLayer,
                             Texture texture,
                             util::Vector3F nxnynz = util::Vector3F(0),
                             util::Vector3F pxpypz = util::Vector3F(1),
                             ColorF color = colorizeIdentityF())
{
    renderCubeFaceNY(renderBuffer,
                     renderLayer,
                     texture,
                     nxnynz,
                     pxpypz,
                     {color, color, color, color, color, color, color, color});
}

inline void renderCubeFaceNY(RenderBuffer &renderBuffer,
                             RenderLayer renderLayer,
                             Texture texture,
                             util::Vector3F nxnynz,
                             util::Vector3F pxpypz,
                             ColorF color,
                             const Transform &tform)
{
    renderCubeFaceNY(renderBuffer,
                     renderLayer,
                     texture,
                     nxnynz,
                     pxpypz,
                     {color, color, color, color, color, color, color, color},
                     tform);
}

inline void renderCubeFacePY(RenderBuffer &renderBuffer,
                             RenderLayer renderLayer,
                             Texture texture,
                             util::Vector3F nxnynz = util::Vector3F(0),
                             util::Vector3F pxpypz = util::Vector3F(1),
                             ColorF color = colorizeIdentityF())
{
    renderCubeFacePY(renderBuffer,
                     renderLayer,
                     texture,
                     nxnynz,
                     pxpypz,
                     {color, color, color, color, color, color, color, color});
}

inline void renderCubeFacePY(RenderBuffer &renderBuffer,
                             RenderLayer renderLayer,
                             Texture texture,
                             util::Vector3F nxnynz,
                             util::Vector3F pxpypz,
                             ColorF color,
                             const Transform &tform)
{
    renderCubeFacePY(renderBuffer,
                     renderLayer,
                     texture,
                     nxnynz,
                     pxpypz,
                     {color, color, color, color, color, color, color, color},
                     tform);
}

inline void renderCubeFaceNZ(RenderBuffer &renderBuffer,
                             RenderLayer renderLayer,
                             Texture texture,
                             util::Vector3F nxnynz = util::Vector3F(0),
                             util::Vector3F pxpypz = util::Vector3F(1),
                             ColorF color = colorizeIdentityF())
{
    renderCubeFaceNZ(renderBuffer,
                     renderLayer,
                     texture,
                     nxnynz,
                     pxpypz,
                     {color, color, color, color, color, color, color, color});
}

inline void renderCubeFaceNZ(RenderBuffer &renderBuffer,
                             RenderLayer renderLayer,
                             Texture texture,
                             util::Vector3F nxnynz,
                             util::Vector3F pxpypz,
                             ColorF color,
                             const Transform &tform)
{
    renderCubeFaceNZ(renderBuffer,
                     renderLayer,
                     texture,
                     nxnynz,
                     pxpypz,
                     {color, color, color, color, color, color, color, color},
                     tform);
}

inline void renderCubeFacePZ(RenderBuffer &renderBuffer,
                             RenderLayer renderLayer,
                             Texture texture,
                             util::Vector3F nxnynz = util::Vector3F(0),
                             util::Vector3F pxpypz = util::Vector3F(1),
                             ColorF color = colorizeIdentityF())
{
    renderCubeFacePZ(renderBuffer,
                     renderLayer,
                     texture,
                     nxnynz,
                     pxpypz,
                     {color, color, color, color, color, color, color, color});
}

inline void renderCubeFacePZ(RenderBuffer &renderBuffer,
                             RenderLayer renderLayer,
                             Texture texture,
                             util::Vector3F nxnynz,
                             util::Vector3F pxpypz,
                             ColorF color,
                             const Transform &tform)
{
    renderCubeFacePZ(renderBuffer,
                     renderLayer,
                     texture,
                     nxnynz,
                     pxpypz,
                     {color, color, color, color, color, color, color, color},
                     tform);
}

inline void renderCube(RenderBuffer &renderBuffer,
                       RenderLayer renderLayer,
                       util::EnumArray<Texture, block::BlockFace> textures,
                       util::EnumArray<bool, block::BlockFace> faceFlags,
                       util::Vector3F nxnynz = util::Vector3F(0),
                       util::Vector3F pxpypz = util::Vector3F(1),
                       ColorF color = colorizeIdentityF())
{
    renderCube(renderBuffer,
               renderLayer,
               textures,
               faceFlags,
               nxnynz,
               pxpypz,
               {color, color, color, color, color, color, color, color});
}

inline void renderCube(RenderBuffer &renderBuffer,
                       RenderLayer renderLayer,
                       util::EnumArray<Texture, block::BlockFace> textures,
                       util::EnumArray<bool, block::BlockFace> faceFlags,
                       util::Vector3F nxnynz,
                       util::Vector3F pxpypz,
                       ColorF color,
                       const Transform &tform)
{
    renderCube(renderBuffer,
               renderLayer,
               textures,
               faceFlags,
               nxnynz,
               pxpypz,
               {color, color, color, color, color, color, color, color},
               tform);
}

inline void renderCubeFace(RenderBuffer &renderBuffer,
                           RenderLayer renderLayer,
                           block::BlockFace face,
                           util::EnumArray<Texture, block::BlockFace> textures,
                           util::Vector3F nxnynz = util::Vector3F(0),
                           util::Vector3F pxpypz = util::Vector3F(1),
                           ColorF color = colorizeIdentityF())
{
    switch(face)
    {
    case block::BlockFace::NX:
        renderCubeFaceNX(
            renderBuffer, renderLayer, textures[block::BlockFace::NX], nxnynz, pxpypz, color);
        return;
    case block::BlockFace::PX:
        renderCubeFacePX(
            renderBuffer, renderLayer, textures[block::BlockFace::PX], nxnynz, pxpypz, color);
        return;
    case block::BlockFace::NY:
        renderCubeFaceNY(
            renderBuffer, renderLayer, textures[block::BlockFace::NY], nxnynz, pxpypz, color);
        return;
    case block::BlockFace::PY:
        renderCubeFacePY(
            renderBuffer, renderLayer, textures[block::BlockFace::PY], nxnynz, pxpypz, color);
        return;
    case block::BlockFace::NZ:
        renderCubeFaceNZ(
            renderBuffer, renderLayer, textures[block::BlockFace::NZ], nxnynz, pxpypz, color);
        return;
    case block::BlockFace::PZ:
        renderCubeFacePZ(
            renderBuffer, renderLayer, textures[block::BlockFace::PZ], nxnynz, pxpypz, color);
        return;
    }
    constexprAssert(false);
}

inline void renderCubeFace(RenderBuffer &renderBuffer,
                           RenderLayer renderLayer,
                           block::BlockFace face,
                           util::EnumArray<Texture, block::BlockFace> textures,
                           util::Vector3F nxnynz,
                           util::Vector3F pxpypz,
                           ColorF color,
                           const Transform &tform)
{
    switch(face)
    {
    case block::BlockFace::NX:
        renderCubeFaceNX(renderBuffer,
                         renderLayer,
                         textures[block::BlockFace::NX],
                         nxnynz,
                         pxpypz,
                         color,
                         tform);
        return;
    case block::BlockFace::PX:
        renderCubeFacePX(renderBuffer,
                         renderLayer,
                         textures[block::BlockFace::PX],
                         nxnynz,
                         pxpypz,
                         color,
                         tform);
        return;
    case block::BlockFace::NY:
        renderCubeFaceNY(renderBuffer,
                         renderLayer,
                         textures[block::BlockFace::NY],
                         nxnynz,
                         pxpypz,
                         color,
                         tform);
        return;
    case block::BlockFace::PY:
        renderCubeFacePY(renderBuffer,
                         renderLayer,
                         textures[block::BlockFace::PY],
                         nxnynz,
                         pxpypz,
                         color,
                         tform);
        return;
    case block::BlockFace::NZ:
        renderCubeFaceNZ(renderBuffer,
                         renderLayer,
                         textures[block::BlockFace::NZ],
                         nxnynz,
                         pxpypz,
                         color,
                         tform);
        return;
    case block::BlockFace::PZ:
        renderCubeFacePZ(renderBuffer,
                         renderLayer,
                         textures[block::BlockFace::PZ],
                         nxnynz,
                         pxpypz,
                         color,
                         tform);
        return;
    }
    constexprAssert(false);
}
}
}
}
}

#endif /* GRAPHICS_SHAPE_CUBE_H_ */
