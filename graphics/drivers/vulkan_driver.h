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

#ifndef GRAPHICS_DRIVERS_VULKAN_DRIVER_H_
#define GRAPHICS_DRIVERS_VULKAN_DRIVER_H_

#include "sdl2_driver.h"
#include "../../util/constexpr_assert.h"
#include <memory>

namespace programmerjake
{
namespace voxels
{
namespace graphics
{
namespace drivers
{
class VulkanDriver final : public SDL2Driver
{
private:
    struct Implementation;

private:
    std::shared_ptr<Implementation> implementation;

private:
    std::shared_ptr<Implementation> createImplementation();

protected:
    virtual void renderFrame(std::shared_ptr<CommandBuffer> commandBuffer) override;
    virtual SDL_Window *createWindow(int x, int y, int w, int h, std::uint32_t flags) override;
    virtual void createGraphicsContext() override;
    virtual void destroyGraphicsContext() noexcept override;

public:
    virtual TextureId makeTexture(const std::shared_ptr<const Image> &image) override;
    virtual void setNewImageData(TextureId texture,
                                 const std::shared_ptr<const Image> &image) override;
    virtual std::shared_ptr<RenderBuffer> makeBuffer(
        const util::EnumArray<std::size_t, RenderLayer> &maximumSizes) override;
    virtual std::shared_ptr<CommandBuffer> makeCommandBuffer() override;
    virtual void setGraphicsContextRecreationNeeded() noexcept override;
    VulkanDriver() : SDL2Driver(), implementation(createImplementation())
    {
    }
    explicit VulkanDriver(std::string title)
        : SDL2Driver(std::move(title)), implementation(createImplementation())
    {
    }
};
}
}
}
}

#endif /* GRAPHICS_DRIVERS_VULKAN_DRIVER_H_ */
