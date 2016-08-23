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

#ifndef GRAPHICS_RENDER_H_
#define GRAPHICS_RENDER_H_

#include <memory>
#include "../util/enum.h"

namespace programmerjake
{
namespace voxels
{
namespace graphics
{
enum class RenderLayer
{
    Opaque,
    OpaqueWithHoles,
    Translucent,
    DEFINE_ENUM_MAX(Translucent)
};

class RenderBuffer
{
    virtual ~RenderBuffer() = default;
    static constexpr std::size_t noMaximumAdditionalSize = -1;
    virtual std::size_t getMaximumAdditionalSize(RenderLayer renderLayer) const noexcept;
    virtual void reserveAdditional(RenderLayer renderLayer, std::size_t howManyTriangles);
    virtual void appendTriangle(RenderLayer renderLayer,
#error finish
        );
};
}
}
}

#endif /* GRAPHICS_RENDER_H_ */
