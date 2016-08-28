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

namespace programmerjake
{
namespace voxels
{
namespace graphics
{
class TextureImplementation;
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
}
}
}

#endif /* GRAPHICS_TEXTURE_H_ */
