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
#include "../graphics_util/texture_atlas.h"

namespace programmerjake
{
namespace voxels
{
namespace graphics
{
namespace drivers
{
struct OpenGL1Driver::Implementation final
{
    using graphics_util::texture_atlas::TextureSize;
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
        float positionX;
        float positionY;
        float positionZ;
        std::uint8_t red;
        std::uint8_t green;
        std::uint8_t blue;
        std::uint8_t opacity;
        float textureCoordinateU;
        float textureCoordinateV;
        constexpr OpenGLVertex(const util::Vector3F &position,
                               const ColorU8 &color,
                               const TextureCoordinates &textureCoordinates)
            : positionX(position.x),
              positionY(position.y),
              positionZ(position.z),
              red(color.red),
              green(color.green),
              blue(color.blue),
              opacity(color.opacity),
              textureCoordinateU(textureCoordinates.u),
              textureCoordinateV(textureCoordinates.v)
        {
        }
    };
    struct OpenGLTriangle final
    {
        OpenGLVertex vertices[3];
        constexpr OpenGLTriangle(const OpenGLVertex &v0,
                                 const OpenGLVertex &v1,
                                 const OpenGLVertex &v2)
            : vertices{v0, v1, v2}
        {
        }
    };
    struct OpenGLRenderBuffer final : public RenderBuffer
    {
#error finish
    private:
        struct TriangleBuffer final
        {
            std::unique_ptr<TriangleWithoutNormal[]> buffer;
            std::unique_ptr<OpenGLTriangle[]> openGLTriangles;
            std::size_t bufferSize;
            std::size_t bufferUsed;
            TriangleBuffer(std::size_t size = 0)
                : buffer(size > 0 ? new TriangleWithoutNormal[size] : nullptr),
                  openGLTriangles(size > 0 ? new OpenGLTriangle[size] : nullptr),
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

    public:
        OpenGLRenderBuffer(const util::EnumArray<std::size_t, RenderLayer> &maximumSizes)
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
        virtual void finish() noexcept override
        {
            finished = true;
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
            std::shared_ptr<RenderBuffer> renderBuffer;
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
            Command(const std::shared_ptr<RenderBuffer> &renderBuffer,
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
            commands.push_back(
                Command(renderBuffer, modelTransform, viewTransform, projectionTransform));
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
    TextureSize currentTextureSize;
    GLuint textureId = 0;
    void layoutTextures(std::unique_lock<std::mutex> &lockedTextureLock)
    {
        updatedTextures.clear();
        TextureSize newTextureSize =
            graphics_util::texture_atlas::TextureAtlas<OpenGLTexture,
                                                       &OpenGLTexture::x,
                                                       &OpenGLTexture::y,
                                                       &OpenGLTexture::size>::layout(textures
                                                                                         .begin(),
                                                                                     textures
                                                                                         .end());
        if(textureId == 0 || newTextureSize != currentTextureSize)
        {
            currentTextureSize = newTextureSize;
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
        for(auto &texture : textures)
        {
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
    explicit Implementation(OpenGL1Driver *driver) : driver(*driver)
    {
        auto image = Image::make(1, 1);
        image->setPixel(0, 0, colorizeIdentityU8());
        textures.push_back(OpenGLTexture(image));
    }
    bool libraryLoaded = false;
    bool srgbTexturesSupported = false;
    bool srgbFramebufferSupported = false;

#define OpenGLBaseFunctions()            \
    FN(glEnable, ENABLE)                 \
    FN(glGenTextures, GENTEXTURES)       \
    FN(glDeleteTextures, DELETETEXTURES) \
    FN(glDepthFunc, DEPTHFUNC)           \
    FN(glCullFace, CULLFACE)             \
    FN(glFrontFace, FRONTFACE)           \
    FN(glAlphaFunc, ALPHAFUNC)           \
    FN(glBlendFunc, BLENDFUNC)           \
    FN(glTexEnvi, TEXENVI)               \
    FN(glHint, HINT)                     \
    FN(glBindTexture, BINDTEXTURE)       \
    FN(glEnableClientState, ENABLECLIENTSTATE)

#define FN(a, b)                           \
    typedef decltype(::a) *PFNGL##b##PROC; \
    PFNGL##b##PROC a = nullptr;

    OpenGLBaseFunctions();
#undef FN

    void loadFunctions()
    {
#define FN(a, b)                                                     \
    a = reinterpret_cast<PFNGL##b##PROC>(SDL_GL_GetProcAddress(#a)); \
    if(!a)                                                           \
        throw std::runtime_error("loading " #a " failed");

        OpenGLBaseFunctions();
#undef FN
        srgbTexturesSupported = SDL_GL_ExtensionSupported("GL_EXT_texture_sRGB");
        srgbFramebufferSupported = SDL_GL_ExtensionSupported("GL_ARB_framebuffer_sRGB");
    }
#undef OpenGLBaseFunctions
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
    void renderFrame(std::shared_ptr<CommandBuffer> commandBuffer)
    {
#error finish
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
    SDL_GL_SetAttribute(SDL_GL_FRAMEBUFFER_SRGB_CAPABLE, 1);
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
}

void OpenGL1Driver::destroyGraphicsContext() noexcept
{
    constexprAssert(implementation->openGLContext);
    SDL_GL_DeleteContext(implementation->openGLContext);
    implementation->openGLContext = nullptr;
    implementation->textureId = 0;
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
#error finish
}

std::shared_ptr<Driver::CommandBuffer> OpenGL1Driver::makeCommandBuffer()
{
    return std::make_shared<Implementation::OpenGLCommandBuffer>();
}
}
}
}
}
