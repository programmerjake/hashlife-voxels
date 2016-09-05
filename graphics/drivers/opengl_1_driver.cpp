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
#include <algorithm>

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
    struct TextureSize final
    {
        std::size_t width, height;
        constexpr TextureSize(std::uint32_t width, std::uint32_t height) noexcept : width(width),
                                                                                    height(height)
        {
        }
        explicit TextureSize(const Image &image) noexcept : width(image.width), height(image.height)
        {
        }
        constexpr bool operator==(const TextureSize &rt) const noexcept
        {
            return width == rt.width && height == rt.height;
        }
        constexpr bool operator!=(const TextureSize &rt) const noexcept
        {
            return !operator==(rt);
        }
        constexpr std::size_t getMaxDimension() const noexcept
        {
            return width > height ? width : height;
        }
        constexpr std::size_t getMinDimension() const noexcept
        {
            return width > height ? height : width;
        }

    private:
        constexpr bool lessHelper(std::size_t maxDimension,
                                  std::size_t minDimension,
                                  const TextureSize &rt,
                                  std::size_t rtMaxDimension,
                                  std::size_t rtMinDimension) const noexcept
        {
            return maxDimension != rtMaxDimension ?
                       maxDimension < rtMaxDimension :
                       minDimension == rtMinDimension ? width < rt.width : minDimension
                                                                               < rtMinDimension;
        }

    public:
        constexpr bool operator<(const TextureSize &rt) const noexcept
        {
            return lessHelper(getMaxDimension(),
                              getMinDimension(),
                              rt,
                              rt.getMaxDimension(),
                              rt.getMinDimension());
        }
        constexpr bool operator>(const TextureSize &rt) const noexcept
        {
            return rt.operator<(*this);
        }
        constexpr bool operator<=(const TextureSize &rt) const noexcept
        {
            return !(operator>(rt));
        }
        constexpr bool operator>=(const TextureSize &rt) const noexcept
        {
            return !(operator<(rt));
        }
    };
    struct OpenGLTexture final : public TextureImplementation
    {
        const std::shared_ptr<Image> image;
        std::size_t x, y;
        explicit OpenGLTexture(std::shared_ptr<Image> image) : image(std::move(image)), x(), y()
        {
        }
    };
    struct TextureGroup final
    {
        TextureSize size;
        std::vector<OpenGLTexture *> textures;
        TextureGroup(TextureSize size, std::vector<OpenGLTexture *> textures)
            : size(size), textures(std::move(textures))
        {
        }
    };
    std::mutex textureLock;
    std::list<OpenGLTexture> textures;
    bool textureLayoutNeeded = true;
    static std::uint64_t powerOf2Ceiling(std::uint64_t v) noexcept
    {
        v--;
        v |= v >> 1;
        v |= v >> 2;
        v |= v >> 4;
        v |= v >> 8;
        v |= v >> 16;
        v |= v >> 32;
        return v + 1;
    }
    static std::uint32_t powerOf2Ceiling(std::uint32_t v) noexcept
    {
        v--;
        v |= v >> 1;
        v |= v >> 2;
        v |= v >> 4;
        v |= v >> 8;
        v |= v >> 16;
        return v + 1;
    }
    static std::uint16_t powerOf2Ceiling(std::uint16_t v) noexcept
    {
        v--;
        v |= v >> 1;
        v |= v >> 2;
        v |= v >> 4;
        v |= v >> 8;
        return v + 1;
    }
    static bool textureGroupCompareFunction(const TextureGroup &a, const TextureGroup &b) noexcept
    {
        return a.size > b.size;
    }
    static void layoutTextures(std::list<OpenGLTexture> &textures)
    {
        std::vector<TextureGroup> textureGroups;
        for(OpenGLTexture &texture : textures)
        {
            texture.x = 0;
            texture.y = 0;
            TextureSize size(texture);
            size.width = size.width == 0 ? 1 : powerOf2Ceiling(size.width);
            size.height = size.height == 0 ? 1 : powerOf2Ceiling(size.height);
            textureGroups.push_back(TextureGroup(size, {&texture}));
            std::push_heap(textureGroups.begin(), textureGroups.end(), textureGroupCompareFunction);
        }
        std::vector<TextureGroup> currentGroups;
        while(textureGroups.size() > 1)
        {
            currentGroups.clear();
            auto currentSize = textureGroups.front().size;
            do
            {
                currentGroups.push_back(TextureGroup(currentSize, {}));
                currentGroups.back().textures.swap(textureGroups.front().textures);
                std::pop_heap(
                    textureGroups.begin(), textureGroups.end(), textureGroupCompareFunction);
                textureGroups.pop_back();
            } while(!textureGroups.empty() && currentSize == textureGroups.front().size);
            auto newSize = currentSize;
            if(currentSize.width < currentSize.height)
                newSize.width *= 2;
            else
                newSize.height *= 2;
            while(!currentGroups.empty())
            {
                TextureGroup newGroup(newSize, {});
                newGroup.textures.swap(currentGroups.back().textures);
                currentGroups.pop_back();
                if(!currentGroups.empty())
                {
                    newGroup.textures.reserve(newGroup.textures.size()
                                              + currentGroups.back().textures.size());
                    if(currentSize.width < currentSize.height)
                    {
                        for(auto *texture : currentGroups.back().textures)
                        {
                            texture->x += currentSize.width;
                            newGroup.textures.push_back(texture);
                        }
                    }
                    else
                    {
                        for(auto *texture : currentGroups.back().textures)
                        {
                            texture->y += currentSize.height;
                            newGroup.textures.push_back(texture);
                        }
                    }
                    currentGroups.pop_back();
                }
                textureGroups.push_back(std::move(newGroup));
                std::push_heap(
                    textureGroups.begin(), textureGroups.end(), textureGroupCompareFunction);
            }
        }
    }
    std::atomic_bool contextRecreationNeeded{false};
    OpenGL1Driver &driver;
    SDL_GLContext openGLContext = nullptr;
    explicit Implementation(OpenGL1Driver *driver) : driver(*driver)
    {
    }
    bool libraryLoaded = false;

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
    }
#undef OpenGLBaseFunctions
    void setupContext()
    {
        glEnable(GL_DEPTH_TEST);
        glEnable(GL_TEXTURE_2D);
        glEnable(GL_ALPHA_TEST);
        glEnable(GL_CULL_FACE);
        glEnable(GL_BLEND);
        if(SDL_GL_ExtensionSupported("GL_ARB_framebuffer_sRGB"))
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
#error finish
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
}

void OpenGL1Driver::setGraphicsContextRecreationNeeded() noexcept
{
    implementation->contextRecreationNeeded.store(true, std::memory_order_relaxed);
}

TextureId OpenGL1Driver::makeTexture(const std::shared_ptr<const Image> &image)
{
#error finish
}

void OpenGL1Driver::setNewImageData(TextureId texture, const std::shared_ptr<const Image> &image)
{
#error finish
}

std::shared_ptr<RenderBuffer> OpenGL1Driver::makeBuffer(
    const util::EnumArray<std::size_t, RenderLayer> &maximumSizes)
{
#error finish
}

std::shared_ptr<OpenGL1Driver::CommandBuffer> OpenGL1Driver::makeCommandBuffer()
{
#error finish
}
}
}
}
}
