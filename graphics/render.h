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
#include "triangle.h"
#include "transform.h"
#include "../util/constexpr_assert.h"
#include <initializer_list>
#include <vector>

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

class ReadableRenderBuffer;

class RenderBuffer
{
public:
    virtual ~RenderBuffer() = default;
    static constexpr std::size_t noMaximumAdditionalSize = -1;
    virtual std::size_t getMaximumAdditionalSize(RenderLayer renderLayer) const noexcept = 0;
    virtual void reserveAdditional(RenderLayer renderLayer, std::size_t howManyTriangles) = 0;
    virtual void appendTriangles(RenderLayer renderLayer,
                                 const Triangle *triangles,
                                 std::size_t triangleCount,
                                 const Transform &tform) = 0;
    virtual void appendTriangles(RenderLayer renderLayer,
                                 const Triangle *triangles,
                                 std::size_t triangleCount) = 0;
    void appendTriangles(RenderLayer renderLayer, std::initializer_list<Triangle> triangles)
    {
        appendTriangles(renderLayer, triangles.begin(), triangles.size());
    }
    void appendTriangles(RenderLayer renderLayer,
                         std::initializer_list<Triangle> triangles,
                         const Transform &tform)
    {
        appendTriangles(renderLayer, triangles.begin(), triangles.size(), tform);
    }
    virtual void appendBuffer(const ReadableRenderBuffer &buffer) = 0;
    virtual void appendBuffer(const ReadableRenderBuffer &buffer, const Transform &tform) = 0;
    virtual void finish() noexcept = 0;
    static std::shared_ptr<RenderBuffer> makeGPUBuffer(
        const util::EnumArray<std::size_t, RenderLayer> &maximumSizes);
};

class ReadableRenderBuffer : public RenderBuffer
{
public:
    virtual std::size_t getTriangleCount(RenderLayer renderLayer) const noexcept = 0;
    virtual void readTriangles(RenderLayer renderLayer,
                               Triangle *buffer,
                               std::size_t bufferSize) const noexcept = 0;
    virtual void readTriangles(RenderLayer renderLayer,
                               TriangleWithoutNormal *buffer,
                               std::size_t bufferSize) const noexcept = 0;
};

class MemoryRenderBuffer final : public ReadableRenderBuffer
{
private:
    util::EnumArray<std::vector<Triangle>, RenderLayer> triangleBuffers;

public:
    MemoryRenderBuffer() : triangleBuffers()
    {
    }
    virtual std::size_t getMaximumAdditionalSize(RenderLayer renderLayer) const noexcept override
    {
        return noMaximumAdditionalSize;
    }
    virtual void reserveAdditional(RenderLayer renderLayer, std::size_t howManyTriangles) override
    {
        triangleBuffers[renderLayer].reserve(triangleBuffers[renderLayer].size()
                                             + howManyTriangles);
    }
    virtual void appendTriangles(RenderLayer renderLayer,
                                 const Triangle *triangles,
                                 std::size_t triangleCount) override
    {
        triangleBuffers[renderLayer].insert(
            triangleBuffers[renderLayer].end(), triangles, triangles + triangleCount);
    }
    virtual void appendTriangles(RenderLayer renderLayer,
                                 const Triangle *triangles,
                                 std::size_t triangleCount,
                                 const Transform &tform) override
    {
        auto &triangleBuffer = triangleBuffers[renderLayer];
        triangleBuffer.reserve(triangleBuffer.size() + triangleCount);
        for(std::size_t i = 0; i < triangleCount; i++)
            triangleBuffer.push_back(transform(tform, triangles[i]));
    }
    virtual void appendBuffer(const ReadableRenderBuffer &buffer) override
    {
        constexprAssert(&buffer != this);
        if(auto *memoryBuffer = dynamic_cast<const MemoryRenderBuffer *>(&buffer))
        {
            for(auto renderLayer : util::EnumTraits<RenderLayer>::values)
            {
                triangleBuffers[renderLayer].insert(
                    triangleBuffers[renderLayer].end(),
                    memoryBuffer->triangleBuffers[renderLayer].begin(),
                    memoryBuffer->triangleBuffers[renderLayer].end());
            }
        }
        else
        {
            for(auto renderLayer : util::EnumTraits<RenderLayer>::values)
            {
                auto &triangleBuffer = triangleBuffers[renderLayer];
                auto originalSize = triangleBuffer.size();
                triangleBuffer.resize(originalSize + buffer.getTriangleCount(renderLayer));
                buffer.readTriangles(renderLayer,
                                     &triangleBuffer[originalSize],
                                     triangleBuffer.size() - originalSize);
            }
        }
    }
    virtual void appendBuffer(const ReadableRenderBuffer &buffer, const Transform &tform) override
    {
        constexprAssert(&buffer != this);
        if(auto *memoryBuffer = dynamic_cast<const MemoryRenderBuffer *>(&buffer))
        {
            for(auto renderLayer : util::EnumTraits<RenderLayer>::values)
            {
                triangleBuffers[renderLayer].reserve(
                    triangleBuffers[renderLayer].size()
                    + memoryBuffer->triangleBuffers[renderLayer].size());
                for(auto &triangle : memoryBuffer->triangleBuffers[renderLayer])
                {
                    triangleBuffers[renderLayer].push_back(transform(tform, triangle));
                }
            }
        }
        else
        {
            for(auto renderLayer : util::EnumTraits<RenderLayer>::values)
            {
                auto &triangleBuffer = triangleBuffers[renderLayer];
                auto originalSize = triangleBuffer.size();
                triangleBuffer.resize(originalSize + buffer.getTriangleCount(renderLayer));
                buffer.readTriangles(renderLayer,
                                     &triangleBuffer[originalSize],
                                     triangleBuffer.size() - originalSize);
                for(std::size_t i = originalSize; i < triangleBuffer.size(); i++)
                    triangleBuffer[i] = transform(tform, triangleBuffer[i]);
            }
        }
    }
    virtual std::size_t getTriangleCount(RenderLayer renderLayer) const noexcept override
    {
        return triangleBuffers[renderLayer].size();
    }
    virtual void readTriangles(RenderLayer renderLayer,
                               Triangle *buffer,
                               std::size_t bufferSize) const noexcept override
    {
        auto &triangleBuffer = triangleBuffers[renderLayer];
        constexprAssert(bufferSize <= triangleBuffer.size());
        for(std::size_t i = 0; i < bufferSize; i++)
            buffer[i] = triangleBuffer[i];
    }
    virtual void readTriangles(RenderLayer renderLayer,
                               TriangleWithoutNormal *buffer,
                               std::size_t bufferSize) const noexcept override
    {
        auto &triangleBuffer = triangleBuffers[renderLayer];
        constexprAssert(bufferSize <= triangleBuffer.size());
        for(std::size_t i = 0; i < bufferSize; i++)
            buffer[i] = triangleBuffer[i];
    }
    virtual void finish() noexcept override
    {
    }
};

class SizedMemoryRenderBuffer final : public ReadableRenderBuffer
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
                buffer[location + i] = triangles[i];
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
    SizedMemoryRenderBuffer(const util::EnumArray<std::size_t, RenderLayer> &maximumSizes)
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
    virtual std::size_t getMaximumAdditionalSize(RenderLayer renderLayer) const noexcept override
    {
        return triangleBuffers[renderLayer].sizeLeft();
    }
    virtual void reserveAdditional(RenderLayer renderLayer, std::size_t howManyTriangles) override
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
        constexprAssert(&buffer != this);
        constexprAssert(!finished);
        for(auto renderLayer : util::EnumTraits<RenderLayer>::values)
        {
            auto &triangleBuffer = triangleBuffers[renderLayer];
            std::size_t triangleCount = buffer.getTriangleCount(renderLayer);
            std::size_t location = triangleBuffer.allocateSpace(triangleCount);
            buffer.readTriangles(renderLayer, &triangleBuffer.buffer[location], triangleCount);
        }
    }
    virtual void appendBuffer(const ReadableRenderBuffer &buffer, const Transform &tform) override
    {
        constexprAssert(&buffer != this);
        constexprAssert(!finished);
        for(auto renderLayer : util::EnumTraits<RenderLayer>::values)
        {
            auto &triangleBuffer = triangleBuffers[renderLayer];
            std::size_t triangleCount = buffer.getTriangleCount(renderLayer);
            std::size_t location = triangleBuffer.allocateSpace(triangleCount);
            buffer.readTriangles(renderLayer, &triangleBuffer.buffer[location], triangleCount);
            for(std::size_t i = 0; i < triangleCount; i++)
                triangleBuffer.buffer[location + i] =
                    transform(tform, triangleBuffer.buffer[location + i]);
        }
    }
    virtual std::size_t getTriangleCount(RenderLayer renderLayer) const noexcept override
    {
        return triangleBuffers[renderLayer].bufferUsed;
    }
    virtual void readTriangles(RenderLayer renderLayer,
                               Triangle *buffer,
                               std::size_t bufferSize) const noexcept override
    {
        auto &triangleBuffer = triangleBuffers[renderLayer];
        constexprAssert(bufferSize <= triangleBuffer.bufferUsed);
        for(std::size_t i = 0; i < bufferSize; i++)
            buffer[i] = triangleBuffer.buffer[i];
    }
    virtual void readTriangles(RenderLayer renderLayer,
                               TriangleWithoutNormal *buffer,
                               std::size_t bufferSize) const noexcept override
    {
        auto &triangleBuffer = triangleBuffers[renderLayer];
        constexprAssert(bufferSize <= triangleBuffer.bufferUsed);
        for(std::size_t i = 0; i < bufferSize; i++)
            buffer[i] = triangleBuffer.buffer[i];
    }
    virtual void finish() noexcept override
    {
        finished = true;
    }
};
}
}
}

#endif /* GRAPHICS_RENDER_H_ */
