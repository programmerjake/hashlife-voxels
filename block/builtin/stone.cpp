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
#include "stone.h"
#include "../../graphics/render.h"
#include "../../graphics/shape/cube.h"
#include "../../lighting/lighting.h"
#include "../../resource/resource.h"

namespace programmerjake
{
namespace voxels
{
namespace block
{
namespace builtin
{
GenericStone::GenericStone(std::string name, graphics::TextureId genericStoneTexture)
    : BlockDescriptor(name,
                      lighting::LightProperties::opaque(),
                      BlockedFaces{{true, true, true, true, true, true}},
                      BlockSummary(false, true)),
      genericStoneTexture(genericStoneTexture)
{
}

void GenericStone::render(
    graphics::MemoryRenderBuffer &renderBuffer,
    const BlockStepInput &stepInput,
    const block::BlockStepGlobalState &stepGlobalState,
    const util::EnumArray<const lighting::BlockLighting *, BlockFace> &blockLightingForFaces,
    const lighting::BlockLighting &blockLightingForCenter,
    const graphics::Transform &transform) const
{
    graphics::MemoryRenderBuffer localRenderBuffer;
    for(BlockFace blockFace : util::EnumTraits<BlockFace>::values)
    {
        if(needRenderBlockFace(stepInput[getDirection(blockFace)].getBlockKind(), blockFace))
        {
            auto offset = getDirection(blockFace);
            auto offsetF = static_cast<util::Vector3F>(offset);
            auto *blockLighting = blockLightingForFaces[blockFace];
            graphics::shape::renderCubeFace(
                localRenderBuffer,
                graphics::RenderLayer::Opaque,
                blockFace,
                util::EnumArray<graphics::Texture, block::BlockFace>{genericStoneTexture,
                                                                     genericStoneTexture,
                                                                     genericStoneTexture,
                                                                     genericStoneTexture,
                                                                     genericStoneTexture,
                                                                     genericStoneTexture});
            localRenderBuffer.applyLight(
                [&](util::Vector3F position, graphics::ColorF color, util::Vector3F normal)
                    -> graphics::ColorF
                {
                    return blockLighting->lightVertex(position - offsetF, color, normal);
                });
            renderBuffer.appendBuffer(localRenderBuffer, transform);
            localRenderBuffer.clear();
        }
    }
}

Stone::Stone() : GenericStone("builtin.stone", resource::readResourceTexture("builtin/stone.png"))
{
}
}
}
}
}
