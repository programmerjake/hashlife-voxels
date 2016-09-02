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

#ifndef GRAPHICS_DRIVERS_OPENGL_1_H_
#define GRAPHICS_DRIVERS_OPENGL_1_H_

#include "../driver.h"
#include "../../util/constexpr_assert.h"

namespace programmerjake
{
namespace voxels
{
namespace graphics
{
namespace drivers
{
class OpenGL1Driver final : public Driver
{
private:
    struct Implementation;

public:
    virtual TextureId makeTexture(const std::shared_ptr<const Image> &image) override
    {
        return TextureId(new NullTextureImplementation(image->width, image->height));
    }
    virtual void setNewImageData(TextureId texture,
                                 const std::shared_ptr<const Image> &image) override
    {
        constexprAssert(dynamic_cast<NullTextureImplementation *>(texture.value));
        constexprAssert(static_cast<NullTextureImplementation *>(texture.value)->width
                        == image->width);
        constexprAssert(static_cast<NullTextureImplementation *>(texture.value)->height
                        == image->height);
    }
    virtual std::shared_ptr<RenderBuffer> makeBuffer(
        const util::EnumArray<std::size_t, RenderLayer> &maximumSizes) override
    {
        return std::make_shared<NullRenderBuffer>(maximumSizes);
    }
    virtual void resume() override
    {
        constexprAssert(!running);
        running = true;
    }
    virtual void pause() override
    {
        constexprAssert(running);
        running = false;
    }
    virtual std::shared_ptr<CommandBuffer> makeCommandBuffer() override
    {
        return std::make_shared<NullCommandBuffer>();
    }
    virtual void submitCommandBuffer(std::shared_ptr<CommandBuffer> commandBuffer) override
    {
        constexprAssert(running);
        constexprAssert(dynamic_cast<const NullCommandBuffer *>(commandBuffer.get()));
        constexprAssert(static_cast<const NullCommandBuffer *>(commandBuffer.get())->finished);
    }
};
}
}
}
}

#endif /* GRAPHICS_DRIVERS_OPENGL_1_H_ */
