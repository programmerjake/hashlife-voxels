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

#ifndef GRAPHICS_IMAGE_H_
#define GRAPHICS_IMAGE_H_

#include <memory>
#include <cstdint>
#include "color.h"

namespace programmerjake
{
namespace voxels
{
namespace io
{
struct InputStream;
}
namespace graphics
{
class Image final
{
    Image(const Image &) = delete;
    Image &operator=(const Image &) = delete;

private:
    struct PrivateAccess final
    {
        friend class Image;

    private:
        PrivateAccess() = default;
    };

private:
    std::uint8_t *pixels;

public:
    class Loader
    {
        friend class Image;

    private:
        mutable const Loader *next = nullptr;
        static const Loader *head;

    public:
        virtual ~Loader() = default;
        const std::size_t signatureSize;
        explicit Loader(std::size_t signatureSize) : signatureSize(signatureSize)
        {
        }
        virtual bool signatureMatches(const unsigned char *bytes, std::size_t byteCount) const
            noexcept = 0;
        virtual std::shared_ptr<Image> load(
            const std::shared_ptr<io::InputStream> &inputStream) const = 0;
    };

public:
    const std::size_t width;
    const std::size_t height;
    static constexpr std::size_t bytesPerPixel = 4;

public:
    Image(std::size_t width, std::size_t height, PrivateAccess)
        : pixels(nullptr), width(width), height(height)
    {
    }
    Image(const Image &other, PrivateAccess)
        : pixels(new std::uint8_t[other.width * other.height * bytesPerPixel]),
          width(other.width),
          height(other.height)
    {
        for(std::size_t i = 0; i < other.width * other.height * bytesPerPixel; i++)
            pixels[i] = other.pixels[i];
    }
    ~Image()
    {
        delete[] pixels;
    }
    std::shared_ptr<Image> duplicate() const
    {
        return std::make_shared<Image>(*this, PrivateAccess());
    }
    static std::shared_ptr<Image> make(std::size_t width, std::size_t height)
    {
        auto retval = std::make_shared<Image>(width, height, PrivateAccess());
        retval->pixels = new std::uint8_t[width * height * bytesPerPixel];
        for(std::size_t i = 0; i < width * height * bytesPerPixel; i++)
            retval->pixels[i] = 0;
        return retval;
    }
    std::uint8_t *data()
    {
        return pixels;
    }
    const std::uint8_t *data() const
    {
        return pixels;
    }
    std::size_t index(std::size_t x, std::size_t y) const noexcept
    {
        constexprAssert(x < width && y < height);
        return (x + y * width) * bytesPerPixel;
    }
    std::uint8_t *data(std::size_t x, std::size_t y) noexcept
    {
        return pixels + index(x, y);
    }
    const std::uint8_t *data(std::size_t x, std::size_t y) const noexcept
    {
        return pixels + index(x, y);
    }
    void setPixel(std::size_t x, std::size_t y, const ColorU8 &color) noexcept
    {
        auto *pixel = data(x, y);
        pixel[0] = color.red;
        pixel[1] = color.green;
        pixel[2] = color.blue;
        pixel[3] = color.opacity;
    }
    ColorU8 getPixel(std::size_t x, std::size_t y) const noexcept
    {
        auto *pixel = data(x, y);
        return rgbaU8(pixel[0], pixel[1], pixel[2], pixel[3]);
    }
    void copy(const Image &source) noexcept
    {
        constexprAssert(width == source.width && height == source.height);
        for(std::size_t i = 0, end = width * height * bytesPerPixel; i < end; i++)
            pixels[i] = source.pixels[i];
    }
    static std::shared_ptr<Image> load(const std::shared_ptr<io::InputStream> &inputStream);
    static void registerLoader(std::unique_ptr<Loader> loader) noexcept;
    static void init();
};
}
}
}

#endif /* GRAPHICS_IMAGE_H_ */
