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

#ifndef GRAPHICS_DRIVERS_NULL_DRIVER_H_
#define GRAPHICS_DRIVERS_NULL_DRIVER_H_

#include "../driver.h"
#include "../../util/constexpr_assert.h"
#include "../../platform/terminate_handler.h"
#include <atomic>

namespace programmerjake
{
namespace voxels
{
namespace graphics
{
namespace drivers
{
class NullDriver final : public Driver
{
private:
    static int installTerminationRequestHandler()
    {
        platform::setTerminationRequestHandler([]()
                                               {
                                                   getTerminationRequestCountVariable().fetch_add(
                                                       1, std::memory_order_relaxed);
                                               });
        return 0;
    }
    static std::atomic_size_t &getTerminationRequestCountVariable()
    {
        static std::atomic_size_t retval(0);
        static int unused = installTerminationRequestHandler();
        static_cast<void>(unused);
        return retval;
    }
    static std::size_t getTerminationRequestCount()
    {
        return getTerminationRequestCountVariable().exchange(0, std::memory_order_relaxed);
    }
    bool running = false;
    struct NullTextureImplementation final : public TextureImplementation
    {
        const std::size_t width;
        const std::size_t height;
        NullTextureImplementation(std::size_t width, std::size_t height)
            : width(width), height(height)
        {
        }
    };
    struct NullRenderBuffer final : public RenderBuffer
    {
    private:
        struct TriangleBuffer final
        {
            std::unique_ptr<Triangle[]> buffer;
            std::size_t bufferSize;
            std::size_t bufferUsed;
            TriangleBuffer(std::size_t size = 0)
                : buffer(size > 0 ? new Triangle[size] : nullptr), bufferSize(size), bufferUsed(0)
            {
            }
            std::size_t allocateSpace(std::size_t triangleCount) noexcept
            {
                constexprAssert(triangleCount <= bufferSize
                                && triangleCount + bufferUsed <= bufferSize);
                auto retval = bufferUsed;
                bufferUsed += triangleCount;
                return retval;
            }
            void append(const Triangle *triangles, std::size_t triangleCount) noexcept
            {
                std::size_t location = allocateSpace(triangleCount);
                for(std::size_t i = 0; i < triangleCount; i++)
                {
                    constexprAssert(triangles[i].texture.value == nullptr
                                    || dynamic_cast<const NullTextureImplementation *>(
                                           triangles[i].texture.value));
                    buffer[location + i] = triangles[i];
                }
            }
            void append(const Triangle &triangle) noexcept
            {
                append(&triangle, 1);
            }
            std::size_t sizeLeft() const noexcept
            {
                return bufferSize - bufferUsed;
            }
        };
        util::EnumArray<TriangleBuffer, RenderLayer> triangleBuffers;
        bool finished;

    public:
        NullRenderBuffer(const util::EnumArray<std::size_t, RenderLayer> &maximumSizes)
            : triangleBuffers(), finished(false)
        {
            for(RenderLayer renderLayer : util::EnumTraits<RenderLayer>::values)
            {
                triangleBuffers[renderLayer] = TriangleBuffer(maximumSizes[renderLayer]);
            }
        }
        bool isFinished() const noexcept
        {
            return finished;
        }
        virtual std::size_t getMaximumAdditionalSize(RenderLayer renderLayer) const
            noexcept override
        {
            return triangleBuffers[renderLayer].sizeLeft();
        }
        virtual void reserveAdditional(RenderLayer renderLayer,
                                       std::size_t howManyTriangles) override
        {
            constexprAssert(!finished);
        }
        virtual void appendTriangles(RenderLayer renderLayer,
                                     const Triangle *triangles,
                                     std::size_t triangleCount) override
        {
            constexprAssert(!finished);
            triangleBuffers[renderLayer].append(triangles, triangleCount);
        }
        virtual void appendTriangles(RenderLayer renderLayer,
                                     const Triangle *triangles,
                                     std::size_t triangleCount,
                                     const Transform &tform) override
        {
            constexprAssert(!finished);
            for(std::size_t i = 0; i < triangleCount; i++)
                triangleBuffers[renderLayer].append(transform(tform, triangles[i]));
        }
        virtual void appendBuffer(const ReadableRenderBuffer &buffer) override
        {
            constexprAssert(!finished);
            for(auto renderLayer : util::EnumTraits<RenderLayer>::values)
            {
                auto &triangleBuffer = triangleBuffers[renderLayer];
                std::size_t triangleCount = buffer.getTriangleCount(renderLayer);
                std::size_t location = triangleBuffer.allocateSpace(triangleCount);
                buffer.readTriangles(renderLayer, &triangleBuffer.buffer[location], triangleCount);
                for(std::size_t i = 0; i < triangleCount; i++)
                {
                    constexprAssert(triangleBuffer.buffer[location + i].texture.value == nullptr
                                    || dynamic_cast<const NullTextureImplementation *>(
                                           triangleBuffer.buffer[location + i].texture.value));
                }
            }
        }
        virtual void appendBuffer(const ReadableRenderBuffer &buffer,
                                  const Transform &tform) override
        {
            constexprAssert(!finished);
            for(auto renderLayer : util::EnumTraits<RenderLayer>::values)
            {
                auto &triangleBuffer = triangleBuffers[renderLayer];
                std::size_t triangleCount = buffer.getTriangleCount(renderLayer);
                std::size_t location = triangleBuffer.allocateSpace(triangleCount);
                buffer.readTriangles(renderLayer, &triangleBuffer.buffer[location], triangleCount);
                for(std::size_t i = 0; i < triangleCount; i++)
                {
                    constexprAssert(triangleBuffer.buffer[location + i].texture.value == nullptr
                                    || dynamic_cast<const NullTextureImplementation *>(
                                           triangleBuffer.buffer[location + i].texture.value));
                    triangleBuffer.buffer[location + i] =
                        transform(tform, triangleBuffer.buffer[location + i]);
                }
            }
        }
        virtual void finish() noexcept override
        {
            finished = true;
        }
    };
    struct NullCommandBuffer final : public CommandBuffer
    {
        bool finished = false;
        virtual void appendClearCommand(bool colorFlag,
                                        bool depthFlag,
                                        const ColorF &backgroundColor) override
        {
            constexprAssert(!finished);
        }
        virtual void appendRenderCommand(const std::shared_ptr<RenderBuffer> &renderBuffer,
                                         const Transform &modelTransform,
                                         const Transform &viewTransform,
                                         const Transform &projectionTransform) override
        {
            constexprAssert(!finished);
            constexprAssert(dynamic_cast<const NullRenderBuffer *>(renderBuffer.get()));
            constexprAssert(
                static_cast<const NullRenderBuffer *>(renderBuffer.get())->isFinished());
        }
        virtual void appendPresentCommandAndFinish() override
        {
            constexprAssert(!finished);
            finished = true;
        }
    };

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
    virtual std::shared_ptr<CommandBuffer> makeCommandBuffer() override
    {
        return std::make_shared<NullCommandBuffer>();
    }
    virtual void run(std::shared_ptr<CommandBuffer>(*renderCallback)(void *arg),
                     void *renderCallbackArg,
                     void (*eventCallback)(void *arg, const ui::event::Event &event),
                     void *eventCallbackArg) override
    {
        running = true;
        try
        {
            while(true)
            {
                auto commandBuffer = renderCallback(renderCallbackArg);
                if(commandBuffer)
                {
                    constexprAssert(dynamic_cast<const NullCommandBuffer *>(commandBuffer.get()));
                    constexprAssert(
                        static_cast<const NullCommandBuffer *>(commandBuffer.get())->finished);
                }
                auto terminationRequestCount = getTerminationRequestCount();
                for(std::size_t i = 0; i < terminationRequestCount; i++)
                {
                    eventCallback(eventCallbackArg, ui::event::Quit());
                }
            }
        }
        catch(...)
        {
            running = false;
            throw;
        }
        running = false;
    }
};
}
}
}
}

#endif /* GRAPHICS_DRIVERS_NULL_DRIVER_H_ */
