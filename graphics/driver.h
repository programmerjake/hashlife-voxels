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

#ifndef GRAPHICS_DRIVER_H_
#define GRAPHICS_DRIVER_H_

#include "image.h"
#include "texture.h"
#include "../util/enum.h"
#include "render.h"
#include "color.h"
#include "transform.h"
#include <memory>
#include <type_traits>
#include <utility>
#include <string>
#include <tuple>
#include "../ui/event.h"

namespace programmerjake
{
namespace voxels
{
namespace graphics
{
class Driver
{
public:
    struct CommandBuffer
    {
        virtual ~CommandBuffer() = default;
        virtual void appendClearCommand(bool colorFlag,
                                        bool depthFlag,
                                        const ColorF &backgroundColor) = 0;
        virtual void appendRenderCommand(const std::shared_ptr<RenderBuffer> &renderBuffer,
                                         const Transform &modelTransform,
                                         const Transform &viewTransform,
                                         const Transform &projectionTransform) = 0;
        virtual void appendPresentCommandAndFinish() = 0;
    };

private:
    static Driver &get(Driver *driver);

public:
    virtual ~Driver() = default;
    static Driver &get()
    {
        return get(nullptr);
    }
    static void init(Driver *driver)
    {
        get(driver);
    }
    virtual TextureId makeTexture(const std::shared_ptr<const Image> &image) = 0;
    virtual void setNewImageData(TextureId texture, const std::shared_ptr<const Image> &image) = 0;
    virtual std::shared_ptr<RenderBuffer> makeBuffer(
        const util::EnumArray<std::size_t, RenderLayer> &maximumSizes) = 0;
    virtual void run(std::shared_ptr<CommandBuffer>(*renderCallback)(void *arg),
                     void *renderCallbackArg,
                     void (*eventCallback)(void *arg, const ui::event::Event &event),
                     void *eventCallbackArg) = 0;
    template <typename RenderCallback, typename EventCallback>
    void run(RenderCallback &&renderCallback, EventCallback &&eventCallback)
    {
        run(
            [](void *arg) -> std::shared_ptr<CommandBuffer>
            {
                return std::forward<RenderCallback>(
                    *static_cast<typename std::remove_reference<RenderCallback>::type *>(arg))();
            },
            const_cast<char *>(&reinterpret_cast<const volatile char &>(renderCallback)),
            [](void *arg, const ui::event::Event &event) -> void
            {
                std::forward<EventCallback>(
                    *static_cast<typename std::remove_reference<EventCallback>::type *>(arg))(
                    event);
            },
            const_cast<char *>(&reinterpret_cast<const volatile char &>(eventCallback)));
    }
    virtual std::shared_ptr<CommandBuffer> makeCommandBuffer() = 0;
    virtual std::pair<std::size_t, std::size_t> getOutputSize() const noexcept = 0;
};
}
}
}

#endif /* GRAPHICS_DRIVER_H_ */
