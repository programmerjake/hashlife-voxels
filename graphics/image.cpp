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
#include "image.h"
#include "../io/input_stream.h"
#include "../util/constexpr_assert.h"
#include "image_loader/png_image_loader.h"
#include "../io/memory_stream.h"
#include "../io/concat_stream.h"

namespace programmerjake
{
namespace voxels
{
namespace graphics
{
const Image::Loader *Image::Loader::head = nullptr;

std::shared_ptr<Image> Image::load(const std::shared_ptr<io::InputStream> &inputStream)
{
    std::size_t signatureSize = 1;
    for(auto *loader = Loader::head; loader != nullptr; loader = loader->next)
    {
        if(loader->signatureSize > signatureSize)
            signatureSize = loader->signatureSize;
    }
    std::vector<unsigned char> signatureBuffer;
    signatureBuffer.resize(signatureSize);
    signatureBuffer.resize(inputStream->readAllBytes(signatureBuffer.data(), signatureSize, false));
    if(signatureBuffer.empty())
        throw io::IOError(std::make_error_code(std::errc::io_error),
                          "Image::load failed: empty file");
    for(auto *loader = Loader::head; loader != nullptr; loader = loader->next)
    {
        if(loader->signatureMatches(signatureBuffer.data(), signatureBuffer.size()))
        {
            return loader->load(std::make_shared<io::ConcatInputStream>(
                std::vector<std::shared_ptr<io::InputStream>>{
                    std::make_shared<io::MemoryInputStream>(std::move(signatureBuffer)),
                    inputStream,
                }));
        }
    }
    throw io::IOError(std::make_error_code(std::errc::io_error),
                      "Image::load failed: unsupported format");
}

void Image::registerLoader(std::unique_ptr<Loader> loader) noexcept
{
    constexprAssert(loader);
    loader->next = Loader::head;
    Loader::head = loader.release();
}

void Image::init()
{
    static bool registeredLoaders = []() -> bool
    {
        registerLoader(std::unique_ptr<Loader>(new image_loader::PNGImageLoader));
        return true;
    };
}
}
}
}
