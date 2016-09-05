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
#include "png_image_loader.h"
#include "png.h"
#include "../../logging/logging.h"
#include "../../io/input_stream.h"
#include <exception>
#include <csetjmp>
#include <type_traits>
#include <cstdint>

namespace programmerjake
{
namespace voxels
{
namespace graphics
{
namespace image_loader
{
PNGImageLoader::PNGImageLoader() : Loader(8)
{
}

bool PNGImageLoader::signatureMatches(const unsigned char *bytes, std::size_t byteCount) const
    noexcept
{
    return ::png_sig_cmp(const_cast<unsigned char *>(bytes), 0, byteCount) == 0;
}

struct PNGImageLoader::Implementation final
{
    std::exception_ptr error;
    bool canLongJump = false;
    std::shared_ptr<Image> image;
    const std::shared_ptr<io::InputStream> &inputStream;
    std::unique_ptr<::png_bytep[]> imageRows;
    Implementation(const std::shared_ptr<io::InputStream> &inputStream) : inputStream(inputStream)
    {
    }
    static void PNGAPI
        errorFunction(::png_structp pngStruct, ::png_const_charp errorMessage) noexcept
    {
        logging::log(logging::Level::Error, "PNGImageLoader", errorMessage);
        Implementation *implementation =
            static_cast<Implementation *>(::png_get_error_ptr(pngStruct));
        try
        {
            throw io::IOError(std::make_error_code(std::errc::io_error), errorMessage);
        }
        catch(...)
        {
            implementation->error = std::current_exception();
        }
        if(!implementation->canLongJump)
            std::terminate();
        std::longjmp(png_jmpbuf(pngStruct), 1);
    }
    static void PNGAPI
        warningFunction(::png_structp pngStruct, ::png_const_charp errorMessage) noexcept
    {
        logging::log(logging::Level::Warning, "PNGImageLoader", errorMessage);
    }
    static void PNGAPI
        readFunction(::png_structp pngStruct, ::png_bytep buffer, ::png_size_t bufferSize) noexcept
    {
        Implementation *implementation = static_cast<Implementation *>(::png_get_io_ptr(pngStruct));
        try
        {
            implementation->inputStream->readAllBytes(reinterpret_cast<::png_bytep>(buffer),
                                                      bufferSize);
        }
        catch(...)
        {
            implementation->error = std::current_exception();
            if(!implementation->canLongJump)
                std::terminate();
            std::longjmp(png_jmpbuf(pngStruct), 1);
        }
    }
};

std::shared_ptr<Image> PNGImageLoader::load(
    const std::shared_ptr<io::InputStream> &inputStream) const
{
    std::unique_ptr<Implementation> implementation(new Implementation(inputStream));
    ::png_structp pngStruct = ::png_create_read_struct(PNG_LIBPNG_VER_STRING,
                                                       static_cast<void *>(implementation.get()),
                                                       &Implementation::errorFunction,
                                                       &Implementation::warningFunction);
    if(!pngStruct)
    {
        if(implementation->error)
            std::rethrow_exception(implementation->error);
        throw std::bad_alloc();
    }
    auto pngInfoDeleter = [&](::png_infop pngInfo) noexcept
    {
        if(pngInfo)
            ::png_destroy_read_struct(&pngStruct, &pngInfo, nullptr);
    };
    std::unique_ptr<::png_info, decltype(pngInfoDeleter)> pngInfo(
        ::png_create_info_struct(pngStruct), pngInfoDeleter);
    if(!pngInfo)
    {
        ::png_destroy_read_struct(&pngStruct, nullptr, nullptr);
        if(implementation->error)
            std::rethrow_exception(implementation->error);
        throw std::bad_alloc();
    }
    implementation->canLongJump = true;
    if(setjmp(png_jmpbuf(pngStruct)))
        std::rethrow_exception(implementation->error);
    ::png_set_read_fn(
        pngStruct, static_cast<void *>(implementation.get()), &Implementation::readFunction);
    ::png_read_info(pngStruct, pngInfo.get());
    implementation->image = Image::make(::png_get_image_width(pngStruct, pngInfo.get()),
                                        ::png_get_image_height(pngStruct, pngInfo.get()));
    implementation->imageRows.reset(
        new ::png_bytep[(::png_get_image_height(pngStruct, pngInfo.get()))]);
    for(std::size_t i = 0; i < implementation->image->height; i++)
    {
        static_assert(std::is_same<unsigned char, std::uint8_t>::value, "");
        implementation->imageRows[i] = reinterpret_cast<::png_bytep>(
            implementation->image->data(0, implementation->image->height - i - 1));
    }
    ::png_set_strip_16(pngStruct);
    ::png_set_packing(pngStruct);
    if(::png_get_color_type(pngStruct, pngInfo.get()) == PNG_COLOR_TYPE_PALETTE)
        ::png_set_palette_to_rgb(pngStruct);
    if((::png_get_color_type(pngStruct, pngInfo.get()) & PNG_COLOR_MASK_ALPHA) == 0)
        ::png_set_filler(pngStruct, 0, PNG_FILLER_AFTER);
    ::png_read_image(pngStruct, implementation->imageRows.get());
    ::png_read_end(pngStruct, pngInfo.get());
    return std::move(implementation->image);
}
}
}
}
}
