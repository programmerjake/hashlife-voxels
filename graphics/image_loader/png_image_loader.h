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

#ifndef GRAPHICS_IMAGE_LOADER_PNG_IMAGE_LOADER_H_
#define GRAPHICS_IMAGE_LOADER_PNG_IMAGE_LOADER_H_

#include "../image.h"

namespace programmerjake
{
namespace voxels
{
namespace graphics
{
namespace image_loader
{
struct PNGImageLoader final : public Image::Loader
{
    PNGImageLoader();
    virtual bool signatureMatches(const unsigned char *bytes, std::size_t byteCount) const
        noexcept override;
    virtual std::shared_ptr<Image> load(
        const std::shared_ptr<io::InputStream> &inputStream) const override;

private:
    struct Implementation;
};
}
}
}
}

#endif /* GRAPHICS_IMAGE_LOADER_PNG_IMAGE_LOADER_H_ */
