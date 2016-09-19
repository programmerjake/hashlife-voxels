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
#include "ray_casting.h"
#include <cmath>

namespace programmerjake
{
namespace voxels
{
namespace util
{
namespace ray_casting
{
#ifndef _MSC_VER
constexpr float Ray::eps;
#endif

namespace
{
void initRayBlockIteratorDimension(float rayDirection,
                                   float rayStartPosition,
                                   int currentPosition,
                                   float &nextRayIntersectionT,
                                   float &rayIntersectionStepT,
                                   int &positionDelta)
{
    if(rayDirection == 0)
        return;
    float invDirection = 1 / rayDirection;
    rayIntersectionStepT = std::fabs(invDirection);
    std::int32_t targetPosition;
    if(rayDirection < 0)
    {
        targetPosition = std::ceil(rayStartPosition) - 1;
        positionDelta = -1;
    }
    else
    {
        targetPosition = std::floor(rayStartPosition) + 1;
        positionDelta = 1;
    }
    nextRayIntersectionT = (targetPosition - rayStartPosition) * invDirection;
}
}

RayBlockIterator::RayBlockIterator(Ray ray)
    : ray(ray),
      currentValue(Ray::eps, static_cast<world::Position3I32>(ray.startPosition)),
      nextRayIntersectionTValues(0),
      rayIntersectionStepTValues(0),
      positionDeltaValues(0)
{
    auto &currentPosition = std::get<1>(currentValue);
    initRayBlockIteratorDimension(ray.direction.x,
                                  ray.startPosition.x,
                                  currentPosition.x,
                                  nextRayIntersectionTValues.x,
                                  rayIntersectionStepTValues.x,
                                  positionDeltaValues.x);
    initRayBlockIteratorDimension(ray.direction.y,
                                  ray.startPosition.y,
                                  currentPosition.y,
                                  nextRayIntersectionTValues.y,
                                  rayIntersectionStepTValues.y,
                                  positionDeltaValues.y);
    initRayBlockIteratorDimension(ray.direction.z,
                                  ray.startPosition.z,
                                  currentPosition.z,
                                  nextRayIntersectionTValues.z,
                                  rayIntersectionStepTValues.z,
                                  positionDeltaValues.z);
}

const RayBlockIterator &RayBlockIterator::operator++()
{
    auto &currentPosition = std::get<1>(currentValue);
    float &t = std::get<0>(currentValue);
    if(ray.direction.x != 0
       && (ray.direction.y == 0 || nextRayIntersectionTValues.x < nextRayIntersectionTValues.y)
       && (ray.direction.z == 0 || nextRayIntersectionTValues.x < nextRayIntersectionTValues.z))
    {
        t = nextRayIntersectionTValues.x;
        nextRayIntersectionTValues.x += rayIntersectionStepTValues.x;
        currentPosition.x += positionDeltaValues.x;
    }
    else if(ray.direction.y != 0
            && (ray.direction.z == 0
                || nextRayIntersectionTValues.y < nextRayIntersectionTValues.z))
    {
        t = nextRayIntersectionTValues.y;
        nextRayIntersectionTValues.y += rayIntersectionStepTValues.y;
        currentPosition.y += positionDeltaValues.y;
    }
    else if(ray.direction.z != 0)
    {
        t = nextRayIntersectionTValues.z;
        nextRayIntersectionTValues.z += rayIntersectionStepTValues.z;
        currentPosition.z += positionDeltaValues.z;
    }
    return *this;
}
}
}
}
}
