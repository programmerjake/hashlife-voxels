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

#ifndef GRAPHICS_TEXTURE_H_
#define GRAPHICS_TEXTURE_H_

#include <memory>
#include <utility>
#include "texture_coordinates.h"

namespace programmerjake
{
namespace voxels
{
namespace graphics
{
class TextureImplementation
{
protected:
    TextureImplementation() = default;

public:
    virtual ~TextureImplementation() = default;
};

class Image;

struct TextureId final
{
    TextureImplementation *value;
    constexpr explicit TextureId(TextureImplementation *value) : value(value)
    {
    }
    constexpr TextureId() : value()
    {
    }
    static TextureId makeTexture(const std::shared_ptr<const Image> &image);
    void setNewImageData(const std::shared_ptr<const Image> &image) const;
};

struct Texture final
{
    TextureId textureId;
    TextureCoordinates nunv;
    TextureCoordinates pupv;
    constexpr Texture(TextureId textureId)
        : textureId(textureId),
          nunv(TextureCoordinates::minUMinV()),
          pupv(TextureCoordinates::maxUMaxV())
    {
    }
    constexpr Texture() : Texture(TextureId())
    {
    }
    constexpr Texture(TextureId textureId, TextureCoordinates nunv, TextureCoordinates pupv)
        : textureId(textureId), nunv(nunv), pupv(pupv)
    {
    }
};
}
}
}

#endif /* GRAPHICS_TEXTURE_H_ */
