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

#ifndef BLOCK_BUILTIN_GLOWSTONE_H_
#define BLOCK_BUILTIN_GLOWSTONE_H_

#include "../block_descriptor.h"
#include "../../graphics/texture.h"

namespace programmerjake
{
namespace voxels
{
namespace block
{
namespace builtin
{
class Glowstone final : public BlockDescriptor
{
public:
    const graphics::TextureId glowstoneTexture;

private:
    Glowstone();

public:
    static const Glowstone *get()
    {
        static const Glowstone *retval = new Glowstone;
        return retval;
    }
    static void init()
    {
        get();
    }
    virtual void render(
        graphics::MemoryRenderBuffer &renderBuffer,
        const BlockStepInput &stepInput,
        const block::BlockStepGlobalState &stepGlobalState,
        const util::EnumArray<const lighting::BlockLighting *, BlockFace> &blockLightingForFaces,
        const lighting::BlockLighting &blockLightingForCenter,
        const graphics::Transform &transform) const override;
};
}
}
}
}

#endif /* BLOCK_BUILTIN_GLOWSTONE_H_ */