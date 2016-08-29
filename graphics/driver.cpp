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
#include "driver.h"
#include "../util/constexpr_assert.h"

namespace programmerjake
{
namespace voxels
{
namespace graphics
{
class Driver::NullDriver final : public Driver
{
public:
    struct NullTextureImplementation final : public TextureImplementation
    {
        const std::size_t width;
        const std::size_t height;
        NullTextureImplementation(std::size_t width, std::size_t height)
            : width(width), height(height)
        {
        }
    };
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
};

Driver &Driver::get()
{
    static Driver *retval = new NullDriver;
#warning finish
    return *retval;
}
}
}
}
