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
#include "opengl_1_driver.h"
#include <SDL_opengl.h>
#include <atomic>
#include <stdexcept>
#include <list>
#include <vector>
#include <mutex>
#include <sstream>
#include <unordered_map>
#include "../graphics_util/texture_atlas.h"
#include "../../logging/logging.h"

namespace programmerjake
{
namespace voxels
{
namespace graphics
{
namespace drivers
{
using graphics_util::texture_atlas::TextureSize;
struct OpenGL1Driver::Implementation final
{
    struct OpenGLTexture final : public TextureImplementation
    {
        const std::shared_ptr<Image> image;
        std::size_t x, y;
        const TextureSize size;
        bool upToDate = false;
        explicit OpenGLTexture(std::shared_ptr<Image> image)
            : image(std::move(image)), x(), y(), size(*this->image)
        {
        }
    };
    struct OpenGLVertex final
    {
        float position[3];
        std::uint8_t color[4];
        float textureCoordinate[2];
        constexpr OpenGLVertex() : position{}, color{}, textureCoordinate{}
        {
        }
        constexpr OpenGLVertex(const util::Vector3F &position,
                               const ColorU8 &color,
                               const TextureCoordinates &textureCoordinates)
            : position{position.x, position.y, position.z},
              color{color.red, color.green, color.blue, color.opacity},
              textureCoordinate{textureCoordinates.u, textureCoordinates.v}
        {
        }
    };
    struct OpenGLCommandBuffer;
    struct OpenGLRenderBuffer final : public RenderBuffer
    {
        friend struct Implementation;
        friend struct OpenGLCommandBuffer;

    private:
        struct TriangleBuffer final
        {
            std::unique_ptr<TriangleWithoutNormal[]> buffer;
            std::unique_ptr<OpenGLVertex[]> openGLTriangles;
            std::size_t bufferSize;
            std::size_t bufferUsed;
            TriangleBuffer(std::size_t size = 0)
                : buffer(size > 0 ? new TriangleWithoutNormal[size] : nullptr),
                  openGLTriangles(size > 0 ?
                                      new OpenGLVertex[TriangleWithoutNormal::vertexCount * size] :
                                      nullptr),
                  bufferSize(size),
                  bufferUsed(0)
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
                    constexprAssert(
                        triangles[i].texture.value == nullptr
                        || dynamic_cast<const OpenGLTexture *>(triangles[i].texture.value));
                    buffer[location + i] = triangles[i];
                }
            }
            void append(const TriangleWithoutNormal *triangles, std::size_t triangleCount) noexcept
            {
                std::size_t location = allocateSpace(triangleCount);
                for(std::size_t i = 0; i < triangleCount; i++)
                {
                    constexprAssert(
                        triangles[i].texture.value == nullptr
                        || dynamic_cast<const OpenGLTexture *>(triangles[i].texture.value));
                    buffer[location + i] = triangles[i];
                }
            }
            void append(const Triangle &triangle) noexcept
            {
                append(&triangle, 1);
            }
            void append(const TriangleWithoutNormal &triangle) noexcept
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
        OpenGL1Driver &driver;
        std::uint64_t textureLayoutVersion = 0;

    public:
        OpenGLRenderBuffer(const util::EnumArray<std::size_t, RenderLayer> &maximumSizes,
                           OpenGL1Driver &driver)
            : triangleBuffers(), finished(false), driver(driver)
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
                triangleBuffers[renderLayer].append(
                    transform(tform, static_cast<TriangleWithoutNormal>(triangles[i])));
        }
        virtual void appendBuffer(const ReadableRenderBuffer &buffer) override
        {
            constexprAssert(!finished);
            for(auto renderLayer : util::EnumTraits<RenderLayer>::values)
            {
                auto &triangleBuffer = triangleBuffers[renderLayer];
                std::size_t triangleCount = buffer.getTriangleCount(renderLayer);
                if(!triangleCount)
                    continue;
                std::size_t location = triangleBuffer.allocateSpace(triangleCount);
                buffer.readTriangles(renderLayer, &triangleBuffer.buffer[location], triangleCount);
                for(std::size_t i = 0; i < triangleCount; i++)
                {
                    constexprAssert(triangleBuffer.buffer[location + i].texture.value == nullptr
                                    || dynamic_cast<const OpenGLTexture *>(
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
                if(!triangleCount)
                    continue;
                std::size_t location = triangleBuffer.allocateSpace(triangleCount);
                buffer.readTriangles(renderLayer, &triangleBuffer.buffer[location], triangleCount);
                for(std::size_t i = 0; i < triangleCount; i++)
                {
                    constexprAssert(triangleBuffer.buffer[location + i].texture.value == nullptr
                                    || dynamic_cast<const OpenGLTexture *>(
                                           triangleBuffer.buffer[location + i].texture.value));
                    triangleBuffer.buffer[location + i] =
                        transform(tform, triangleBuffer.buffer[location + i]);
                }
            }
        }
        void fillOpenGLTriangles(std::unique_lock<std::mutex> &lockedTextureLock) noexcept
        {
            auto invCurrentTextureWidth = driver.implementation->invCurrentTextureWidth;
            auto invCurrentTextureHeight = driver.implementation->invCurrentTextureHeight;
            auto &blankTexture = driver.implementation->textures.front();
            constexpr float pixelOffsetUnscaled = static_cast<float>(1.0 / 0x4000L);
            const float pixelOffset =
                pixelOffsetUnscaled
                * (invCurrentTextureWidth < invCurrentTextureHeight ? invCurrentTextureWidth :
                                                                      invCurrentTextureHeight);
            const float doublePixelOffset = 2 * pixelOffset;
            textureLayoutVersion = driver.implementation->textureLayoutVersion;
            for(auto renderLayer : util::EnumTraits<RenderLayer>::values)
            {
                auto &triangleBuffer = triangleBuffers[renderLayer];
                for(std::size_t i = 0; i < triangleBuffer.bufferUsed; i++)
                {
                    const TriangleWithoutNormal &triangle = triangleBuffer.buffer[i];
                    auto *texture = static_cast<OpenGLTexture *>(triangle.texture.value);
                    if(!texture)
                        texture = &blankTexture;
                    auto textureSize = texture->size;
                    auto textureOffsetX = texture->x;
                    auto textureOffsetY = texture->y;
                    float adjustedWidth = textureSize.width - doublePixelOffset;
                    float adjustedHeight = textureSize.height - doublePixelOffset;
                    float adjustedOffsetX = textureOffsetX + pixelOffset;
                    float adjustedOffsetY = textureOffsetY + pixelOffset;
                    for(std::size_t j = 0; j < TriangleWithoutNormal::vertexCount; j++)
                    {
                        triangleBuffer.openGLTriangles[i * TriangleWithoutNormal::vertexCount + j] =
                            OpenGLVertex(
                                triangle.vertices[j].getPosition(),
                                static_cast<ColorU8>(triangle.vertices[j].getColor()),
                                TextureCoordinates(
                                    (triangle.vertices[j].textureCoordinatesU * adjustedWidth
                                     + adjustedOffsetX) * invCurrentTextureWidth,
                                    (triangle.vertices[j].textureCoordinatesV * adjustedHeight
                                     + adjustedOffsetY) * invCurrentTextureHeight));
                    }
                }
            }
        }
        void fillOpenGLTrianglesIfNeeded(std::unique_lock<std::mutex> &lockedTextureLock) noexcept
        {
            if(textureLayoutVersion != driver.implementation->textureLayoutVersion)
                fillOpenGLTriangles(lockedTextureLock);
        }
        virtual void finish() noexcept override
        {
            if(finished)
                return;
            finished = true;
            std::unique_lock<std::mutex> lockedTextureLock(driver.implementation->textureLock);
            if(!driver.implementation->textureLayoutNeeded)
                fillOpenGLTriangles(lockedTextureLock);
        }
    };
    struct OpenGLBufferObject final
    {
        std::weak_ptr<OpenGLRenderBuffer> renderBuffer;
        util::EnumArray<GLuint, RenderLayer> openGLBuffers;
        OpenGLBufferObject() : renderBuffer(), openGLBuffers()
        {
        }
        OpenGLBufferObject(std::weak_ptr<OpenGLRenderBuffer> renderBuffer,
                           const util::EnumArray<GLuint, RenderLayer> &openGLBuffers)
            : renderBuffer(std::move(renderBuffer)), openGLBuffers(openGLBuffers)
        {
        }
    };
    struct OpenGLCommandBuffer final : public CommandBuffer
    {
        struct Command final
        {
            enum class Type
            {
                Clear,
                Render,
            };
            Type type;
            bool colorFlag;
            bool depthFlag;
            ColorF backgroundColor;
            std::shared_ptr<OpenGLRenderBuffer> renderBuffer;
            Transform modelTransform;
            Transform viewTransform;
            Transform projectionTransform;
            Command(bool colorFlag, bool depthFlag, ColorF backgroundColor)
                : type(Type::Clear),
                  colorFlag(colorFlag),
                  depthFlag(depthFlag),
                  backgroundColor(backgroundColor),
                  renderBuffer(),
                  modelTransform(),
                  viewTransform(),
                  projectionTransform()
            {
            }
            Command(const std::shared_ptr<OpenGLRenderBuffer> &renderBuffer,
                    const Transform &modelTransform,
                    const Transform &viewTransform,
                    const Transform &projectionTransform)
                : type(Type::Render),
                  colorFlag(),
                  depthFlag(),
                  backgroundColor(),
                  renderBuffer(renderBuffer),
                  modelTransform(modelTransform),
                  viewTransform(viewTransform),
                  projectionTransform(projectionTransform)
            {
            }
        };
        std::vector<Command> commands;
        bool finished = false;
        virtual void appendClearCommand(bool colorFlag,
                                        bool depthFlag,
                                        const ColorF &backgroundColor) override
        {
            constexprAssert(!finished);
            commands.push_back(Command(colorFlag, depthFlag, backgroundColor));
        }
        virtual void appendRenderCommand(const std::shared_ptr<RenderBuffer> &renderBuffer,
                                         const Transform &modelTransform,
                                         const Transform &viewTransform,
                                         const Transform &projectionTransform) override
        {
            constexprAssert(!finished);
            constexprAssert(renderBuffer);
            if(dynamic_cast<const EmptyRenderBuffer *>(renderBuffer.get()))
                return;
            constexprAssert(dynamic_cast<const OpenGLRenderBuffer *>(renderBuffer.get()));
            constexprAssert(static_cast<const OpenGLRenderBuffer *>(renderBuffer.get())->finished);
            commands.push_back(Command(std::static_pointer_cast<OpenGLRenderBuffer>(renderBuffer),
                                       modelTransform,
                                       viewTransform,
                                       projectionTransform));
        }
        virtual void appendPresentCommandAndFinish() override
        {
            constexprAssert(!finished);
            finished = true;
        }
    };
    std::mutex textureLock;
    std::list<OpenGLTexture> textures;
    std::vector<OpenGLTexture *> updatedTextures;
    bool textureLayoutNeeded = true;
    std::uint64_t textureLayoutVersion = 0;
    TextureSize currentTextureSize;
    float invCurrentTextureWidth = 1;
    float invCurrentTextureHeight = 1;
    GLuint textureId = 0;
    void layoutTextures(std::unique_lock<std::mutex> &lockedTextureLock)
    {
        textureLayoutVersion++;
        textureLayoutNeeded = false;
        updatedTextures.clear();
        TextureSize newTextureSize =
            graphics_util::texture_atlas::TextureAtlas<OpenGLTexture,
                                                       &OpenGLTexture::x,
                                                       &OpenGLTexture::y,
                                                       &OpenGLTexture::size>::layout(textures
                                                                                         .begin(),
                                                                                     textures
                                                                                         .end());
        logging::log(logging::Level::Info, "OpenGL1Driver", "laid out textures");
        if(textureId == 0 || newTextureSize != currentTextureSize)
        {
            std::ostringstream ss;
            ss << "reallocating OpenGL texture: " << newTextureSize.width << "x"
               << newTextureSize.height;
            logging::log(logging::Level::Info, "OpenGL1Driver", ss.str());
            currentTextureSize = newTextureSize;
            invCurrentTextureWidth = 1.0f / currentTextureSize.width;
            invCurrentTextureHeight = 1.0f / currentTextureSize.height;
            if(textureId != 0)
                glDeleteTextures(1, &textureId);
            glGenTextures(1, &textureId);
            glBindTexture(GL_TEXTURE_2D, textureId);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            glTexImage2D(GL_TEXTURE_2D,
                         0,
                         srgbTexturesSupported ? GL_SRGB_ALPHA_EXT : GL_RGBA,
                         newTextureSize.width,
                         newTextureSize.height,
                         0,
                         GL_RGBA,
                         GL_UNSIGNED_BYTE,
                         nullptr);
        }
        std::size_t textureIndex = 0;
        for(auto &texture : textures)
        {
            std::ostringstream ss;
            ss << "texture #" << textureIndex++ << ": " << texture.size.width << "x"
               << texture.size.height << " at " << texture.x << ", " << texture.y;
            logging::log(logging::Level::Debug, "OpenGL1Driver", ss.str());
            texture.upToDate = true;
            glTexSubImage2D(GL_TEXTURE_2D,
                            0,
                            texture.x,
                            texture.y,
                            texture.size.width,
                            texture.size.height,
                            GL_RGBA,
                            GL_UNSIGNED_BYTE,
                            texture.image->data());
        }
    }
    void updateTextures(std::unique_lock<std::mutex> &lockedTextureLock)
    {
        if(textureLayoutNeeded)
        {
            layoutTextures(lockedTextureLock);
            return;
        }
        for(auto *texture : updatedTextures)
        {
            texture->upToDate = true;
            glTexSubImage2D(GL_TEXTURE_2D,
                            0,
                            texture->x,
                            texture->y,
                            texture->size.width,
                            texture->size.height,
                            GL_RGBA,
                            GL_UNSIGNED_BYTE,
                            texture->image->data());
        }
    }
    std::atomic_bool contextRecreationNeeded{false};
    OpenGL1Driver &driver;
    SDL_GLContext openGLContext = nullptr;
    std::unordered_map<const OpenGLRenderBuffer *, OpenGLBufferObject>
        renderBufferToOpenGLBufferObjectMap;
    explicit Implementation(OpenGL1Driver *driver) : driver(*driver)
    {
        auto image = Image::make(1, 1);
        image->setPixel(0, 0, colorizeIdentityU8());
        textures.push_back(OpenGLTexture(image));
    }
    bool libraryLoaded = false;
    bool srgbTexturesSupported = false;
    bool srgbFramebufferSupported = false;
    bool buffersSupported = false;

#define OpenGLBaseFunctions()              \
    FN(glEnable, ENABLE)                   \
    FN(glGenTextures, GENTEXTURES)         \
    FN(glDeleteTextures, DELETETEXTURES)   \
    FN(glDepthFunc, DEPTHFUNC)             \
    FN(glCullFace, CULLFACE)               \
    FN(glFrontFace, FRONTFACE)             \
    FN(glAlphaFunc, ALPHAFUNC)             \
    FN(glBlendFunc, BLENDFUNC)             \
    FN(glTexEnvi, TEXENVI)                 \
    FN(glHint, HINT)                       \
    FN(glBindTexture, BINDTEXTURE)         \
    FN(glDisable, DISABLE)                 \
    FN(glTexImage2D, TEXIMAGE2D)           \
    FN(glTexSubImage2D, TEXSUBIMAGE2D)     \
    FN(glTexParameteri, TEXPARAMETERI)     \
    FN(glVertexPointer, VERTEXPOINTER)     \
    FN(glColorPointer, COLORPOINTER)       \
    FN(glTexCoordPointer, TEXCOORDPOINTER) \
    FN(glDrawArrays, DRAWARRAYS)           \
    FN(glClearColor, CLEARCOLOR)           \
    FN(glClear, CLEAR)                     \
    FN(glMatrixMode, MATRIXMODE)           \
    FN(glLoadMatrixf, LOADMATRIXF)         \
    FN(glViewport, VIEWPORT)               \
    FN(glDepthMask, DEPTHMASK)             \
    FN(glEnableClientState, ENABLECLIENTSTATE)

#define OpenGLBufferFunctions()              \
    FN(glBindBufferARB, BINDBUFFERARB)       \
    FN(glDeleteBuffersARB, DELETEBUFFERSARB) \
    FN(glGenBuffersARB, GENBUFFERSARB)       \
    FN(glBufferDataARB, BUFFERDATAARB)

#define FN(a, b)                           \
    typedef decltype(::a) *PFNGL##b##PROC; \
    PFNGL##b##PROC a = nullptr;

    OpenGLBaseFunctions();
#undef FN

#define FN(a, b) PFNGL##b##PROC a = nullptr;

    OpenGLBufferFunctions();
#undef FN

    void loadFunctions()
    {
#define FN(a, b)                                                     \
    a = reinterpret_cast<PFNGL##b##PROC>(SDL_GL_GetProcAddress(#a)); \
    if(!a)                                                           \
        throw std::runtime_error("loading " #a " failed");

        OpenGLBaseFunctions();
        srgbTexturesSupported = SDL_GL_ExtensionSupported("GL_EXT_texture_sRGB");
        srgbFramebufferSupported = SDL_GL_ExtensionSupported("GL_ARB_framebuffer_sRGB");
        buffersSupported = SDL_GL_ExtensionSupported("GL_ARB_vertex_buffer_object");
        if(buffersSupported)
            OpenGLBufferFunctions();
#undef FN
    }
#undef OpenGLBaseFunctions
#undef OpenGLBufferFunctions
    void setupContext()
    {
        glEnable(GL_DEPTH_TEST);
        glEnable(GL_TEXTURE_2D);
        glEnable(GL_ALPHA_TEST);
        glEnable(GL_CULL_FACE);
        glEnable(GL_BLEND);
        if(srgbFramebufferSupported)
            glEnable(GL_FRAMEBUFFER_SRGB);
        glDepthFunc(GL_LEQUAL);
        glCullFace(GL_BACK);
        glFrontFace(GL_CCW);
        glAlphaFunc(GL_GREATER, 0.0f);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
        glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);
        glEnableClientState(GL_COLOR_ARRAY);
        glEnableClientState(GL_TEXTURE_COORD_ARRAY);
        glEnableClientState(GL_VERTEX_ARRAY);
    }
    void loadMatrix(const util::Matrix4x4F &matrix) noexcept
    {
        float matArray[16] = {matrix[0][0],
                              matrix[0][1],
                              matrix[0][2],
                              matrix[0][3],

                              matrix[1][0],
                              matrix[1][1],
                              matrix[1][2],
                              matrix[1][3],

                              matrix[2][0],
                              matrix[2][1],
                              matrix[2][2],
                              matrix[2][3],

                              matrix[3][0],
                              matrix[3][1],
                              matrix[3][2],
                              matrix[3][3]};
        glLoadMatrixf(static_cast<const float *>(matArray));
    }
    void renderBuffers(const std::vector<OpenGLCommandBuffer::Command *> &renderCommands)
    {
        if(renderCommands.empty())
            return;
        for(RenderLayer renderLayer : util::EnumTraits<RenderLayer>::values)
        {
            switch(renderLayer)
            {
            case RenderLayer::Opaque:
                glDisable(GL_ALPHA_TEST);
                glDisable(GL_BLEND);
                glDepthMask(GL_TRUE);
                break;
            case RenderLayer::OpaqueWithHoles:
                glEnable(GL_ALPHA_TEST);
                glEnable(GL_BLEND);
                glDepthMask(GL_TRUE);
                break;
            case RenderLayer::Translucent:
                glEnable(GL_ALPHA_TEST);
                glEnable(GL_BLEND);
                glDepthMask(GL_FALSE);
                break;
            }
            for(auto *command : renderCommands)
            {
                auto &triangleBuffer = command->renderBuffer->triangleBuffers[renderLayer];
                if(triangleBuffer.bufferUsed == 0)
                    continue;
                glMatrixMode(GL_MODELVIEW);
                loadMatrix(command->modelTransform.positionMatrix.concat(
                    command->viewTransform.positionMatrix));
                glMatrixMode(GL_PROJECTION);
                loadMatrix(command->projectionTransform.positionMatrix);
                const char *bufferBase = nullptr;
                if(buffersSupported)
                {
                    auto &buffer = renderBufferToOpenGLBufferObjectMap[command->renderBuffer.get()];
                    if(buffer.openGLBuffers[renderLayer] == 0)
                    {
                        buffer.renderBuffer = command->renderBuffer;
                        glGenBuffersARB(1, &buffer.openGLBuffers[renderLayer]);
                        glBindBufferARB(GL_ARRAY_BUFFER, buffer.openGLBuffers[renderLayer]);
                        glBufferDataARB(
                            GL_ARRAY_BUFFER,
                            triangleBuffer.bufferUsed * TriangleWithoutNormal::vertexCount
                                * sizeof(OpenGLVertex),
                            static_cast<const void *>(triangleBuffer.openGLTriangles.get()),
                            GL_STATIC_DRAW);
                    }
                    else
                    {
                        glBindBufferARB(GL_ARRAY_BUFFER, buffer.openGLBuffers[renderLayer]);
                    }
                }
                else
                {
                    bufferBase =
                        reinterpret_cast<const char *>(triangleBuffer.openGLTriangles.get());
                }
                glVertexPointer(3,
                                GL_FLOAT,
                                sizeof(OpenGLVertex),
                                bufferBase + offsetof(OpenGLVertex, position));
                glColorPointer(4,
                               GL_UNSIGNED_BYTE,
                               sizeof(OpenGLVertex),
                               bufferBase + offsetof(OpenGLVertex, color));
                glTexCoordPointer(2,
                                  GL_FLOAT,
                                  sizeof(OpenGLVertex),
                                  bufferBase + offsetof(OpenGLVertex, textureCoordinate));
                glDrawArrays(GL_TRIANGLES,
                             0,
                             static_cast<GLsizei>(triangleBuffer.bufferUsed
                                                  * TriangleWithoutNormal::vertexCount));
            }
        }
    }
    void renderFrame(std::shared_ptr<CommandBuffer> commandBufferIn)
    {
        constexprAssert(dynamic_cast<const OpenGLCommandBuffer *>(commandBufferIn.get()));
        auto *commandBuffer = static_cast<OpenGLCommandBuffer *>(commandBufferIn.get());
        constexprAssert(commandBuffer->finished);
        auto outputSize = driver.getOutputSize();
        glViewport(0, 0, std::get<0>(outputSize), std::get<1>(outputSize));
        std::unique_lock<std::mutex> lockedTextureLock(textureLock);
        updateTextures(lockedTextureLock);
        std::vector<OpenGLCommandBuffer::Command *> renderCommands;
        for(auto &command : commandBuffer->commands)
        {
            switch(command.type)
            {
            case OpenGLCommandBuffer::Command::Type::Clear:
                renderBuffers(renderCommands);
                renderCommands.clear();
                glClearColor(command.backgroundColor.red,
                             command.backgroundColor.green,
                             command.backgroundColor.blue,
                             command.backgroundColor.opacity);
                glDepthMask(GL_TRUE);
                glClear((command.colorFlag ? GL_COLOR_BUFFER_BIT : 0)
                        | (command.colorFlag ? GL_DEPTH_BUFFER_BIT : 0));
                break;
            case OpenGLCommandBuffer::Command::Type::Render:
            {
                command.renderBuffer->fillOpenGLTrianglesIfNeeded(lockedTextureLock);
                renderCommands.push_back(&command);
                break;
            }
            }
        }
        renderBuffers(renderCommands);
        renderCommands.clear();
        SDL_GL_SwapWindow(driver.getWindow());
        for(auto iter = renderBufferToOpenGLBufferObjectMap.begin();
            iter != renderBufferToOpenGLBufferObjectMap.end();)
        {
            auto &buffer = std::get<1>(*iter);
            if(buffer.renderBuffer.expired())
            {
                glDeleteBuffersARB(buffer.openGLBuffers.size(), buffer.openGLBuffers.data());
                iter = renderBufferToOpenGLBufferObjectMap.erase(iter);
            }
            else
            {
                ++iter;
            }
        }
    }
};

std::shared_ptr<OpenGL1Driver::Implementation> OpenGL1Driver::createImplementation()
{
    return std::make_shared<Implementation>(this);
}

void OpenGL1Driver::renderFrame(std::shared_ptr<CommandBuffer> commandBuffer)
{
    if(implementation->contextRecreationNeeded.exchange(false, std::memory_order_relaxed))
    {
        destroyGraphicsContext();
        createGraphicsContext();
    }
    implementation->renderFrame(std::move(commandBuffer));
}

SDL_Window *OpenGL1Driver::createWindow(int x, int y, int w, int h, std::uint32_t flags)
{
    if(!implementation->libraryLoaded)
    {
        if(0 != SDL_GL_LoadLibrary(nullptr))
            throw std::runtime_error(std::string("SDL_GL_LoadLibrary failed: ") + SDL_GetError());
        implementation->libraryLoaded = true;
    }
    SDL_GL_ResetAttributes();
    auto window = SDL_CreateWindow(getTitle().c_str(), x, y, w, h, flags | SDL_WINDOW_OPENGL);
    if(window)
        return window;
    throw std::runtime_error(std::string("SDL_CreateWindow failed: ") + SDL_GetError());
}

void OpenGL1Driver::createGraphicsContext()
{
    constexprAssert(!implementation->openGLContext);
    implementation->openGLContext = SDL_GL_CreateContext(getWindow());
    if(!implementation->openGLContext)
        throw std::runtime_error(std::string("SDL_GL_CreateContext failed: ") + SDL_GetError());
    implementation->loadFunctions();
    implementation->setupContext();
    logging::log(logging::Level::Info, "OpenGL1Driver", "created graphics context");
}

void OpenGL1Driver::destroyGraphicsContext() noexcept
{
    constexprAssert(implementation->openGLContext);
    SDL_GL_DeleteContext(implementation->openGLContext);
    implementation->openGLContext = nullptr;
    implementation->textureId = 0;
    implementation->renderBufferToOpenGLBufferObjectMap.clear();
    std::unique_lock<std::mutex> lockedTextureLock(implementation->textureLock);
    implementation->textureLayoutNeeded = true;
    lockedTextureLock.unlock();
    logging::log(logging::Level::Info, "OpenGL1Driver", "destroyed graphics context");
}

void OpenGL1Driver::setGraphicsContextRecreationNeeded() noexcept
{
    implementation->contextRecreationNeeded.store(true, std::memory_order_relaxed);
}

TextureId OpenGL1Driver::makeTexture(const std::shared_ptr<const Image> &image)
{
    constexprAssert(image);
    std::unique_lock<std::mutex> lockedTextureLock(implementation->textureLock);
    implementation->textureLayoutNeeded = true;
    implementation->textures.push_back(Implementation::OpenGLTexture(image->duplicate()));
    return TextureId(&implementation->textures.back());
}

void OpenGL1Driver::setNewImageData(TextureId texture, const std::shared_ptr<const Image> &image)
{
    constexprAssert(image);
    constexprAssert(texture.value);
    constexprAssert(dynamic_cast<Implementation::OpenGLTexture *>(texture.value));
    std::unique_lock<std::mutex> lockedTextureLock(implementation->textureLock);
    auto openGLTexture = static_cast<Implementation::OpenGLTexture *>(texture.value);
    openGLTexture->image->copy(*image);
    if(openGLTexture->upToDate)
    {
        openGLTexture->upToDate = false;
        implementation->updatedTextures.push_back(openGLTexture);
    }
}

std::shared_ptr<RenderBuffer> OpenGL1Driver::makeBuffer(
    const util::EnumArray<std::size_t, RenderLayer> &maximumSizes)
{
    return std::make_shared<Implementation::OpenGLRenderBuffer>(maximumSizes, *this);
}

std::shared_ptr<CommandBuffer> OpenGL1Driver::makeCommandBuffer()
{
    return std::make_shared<Implementation::OpenGLCommandBuffer>();
}
}
}
}
}
