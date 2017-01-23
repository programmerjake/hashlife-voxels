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
#include "dimension.h"
#include "../logging/logging.h"
#include <exception>
#include "../util/constexpr_assert.h"
#include "../lighting/lighting.h"

namespace programmerjake
{
namespace voxels
{
namespace world
{
std::vector<Dimension::Properties> *Dimension::makePropertiesLookupTable() noexcept
{
    auto *retval = new std::vector<Properties>;
    constexprAssert(retval->size() == overworld().value);
    retval->push_back(Properties(0, "Overworld", true, lighting::Lighting::maxLight));
    constexprAssert(retval->size() == nether().value);
    retval->push_back(Properties(0, "Nether", false, 0));
    constexprAssert(retval->size() - 1 == lastPredefinedDimension().value);
    return retval;
}

void Dimension::handleTooManyDimensions() noexcept
{
    logging::log(logging::Level::Fatal, "Dimension", "out of Dimension values");
    std::terminate();
}
}
}
}
