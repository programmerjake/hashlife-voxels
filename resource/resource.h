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

#ifndef RESOURCE_RESOURCE_H_
#define RESOURCE_RESOURCE_H_

#include <memory>
#include <string>
#include "../graphics/texture.h"

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
class Image;
}
namespace resource
{
std::shared_ptr<io::InputStream> readResource(std::string name);
std::shared_ptr<graphics::Image> readResourceImage(std::string name);
graphics::TextureId readResourceTexture(std::string name);
}
}
}

#endif /* RESOURCE_RESOURCE_H_ */
