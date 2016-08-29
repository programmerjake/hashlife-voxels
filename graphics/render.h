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
};
}
}
}

#endif /* GRAPHICS_RENDER_H_ */
