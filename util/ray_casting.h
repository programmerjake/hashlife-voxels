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
#ifndef RAY_CASTING_H_INCLUDED
#define RAY_CASTING_H_INCLUDED

#include "vector.h"
#include "../block/block.h"
#include "../graphics/transform.h"
#include "enum.h"
#include "../world/position.h"
#include "optional.h"
#include <cstdint>
#include <atomic>
#include <tuple>

namespace programmerjake
{
namespace voxels
{
namespace world
{
class World;
}
namespace entity
{
class Entity;
}
namespace util
{
namespace ray_casting
{
typedef std::uint32_t BlockCollisionMask;
constexpr BlockCollisionMask BlockCollisionMaskGround = 1 << 0;
constexpr BlockCollisionMask BlockCollisionMaskFluid = 1 << 1;
constexpr BlockCollisionMask BlockCollisionMaskDefault = ~BlockCollisionMaskFluid;
constexpr BlockCollisionMask BlockCollisionMaskEverything = ~static_cast<BlockCollisionMask>(0);
inline BlockCollisionMask allocateBlockCollisionMask()
{
    static std::atomic_int nextBit(2);
    int bit = nextBit++;
    assert(bit < 32);
    return static_cast<BlockCollisionMask>(1) << bit;
}

struct Ray final
{
    world::Position3F startPosition;
    Vector3F direction;
    static constexpr float eps = 1e-4;
    constexpr Ray(world::Position3F startPosition, Vector3F direction)
        : startPosition(startPosition), direction(direction)
    {
    }
    constexpr Ray() : startPosition(), direction()
    {
    }
    constexpr world::Position3F eval(float t) const
    {
        return startPosition + Vector3F(t) * direction;
    }
    float collideWithPlane(Vector3F planeNormal, float planeD) const
    {
        float divisor = dot(planeNormal, direction);
        if(divisor == 0)
            return -1;
        float numerator = -dot(planeNormal, startPosition) - planeD;
        float retval = numerator / divisor;
        return retval >= eps ? retval : -1;
    }
    float collideWithPlane(std::pair<Vector3F, float> planeEquation) const
    {
        return collideWithPlane(std::get<0>(planeEquation), std::get<1>(planeEquation));
    }

private:
    template <char ignoreAxis>
    static bool isPointInAABoxIgnoreAxis(Vector3F minCorner, Vector3F maxCorner, Vector3F pos)
    {
        static_assert(
            ignoreAxis == 'X' || ignoreAxis == 'Y' || ignoreAxis == 'Z' || ignoreAxis == ' ',
            "invalid ignore axis");
        bool isPointInBoxX = ignoreAxis == 'X' || (pos.x >= minCorner.x && pos.x <= maxCorner.x);
        bool isPointInBoxY = ignoreAxis == 'Y' || (pos.y >= minCorner.y && pos.y <= maxCorner.y);
        bool isPointInBoxZ = ignoreAxis == 'Z' || (pos.z >= minCorner.z && pos.z <= maxCorner.z);
        return isPointInBoxX && isPointInBoxY && isPointInBoxZ;
    }

public:
    static bool isPointInAABox(Vector3F minCorner, Vector3F maxCorner, Vector3F pos)
    {
        return isPointInAABoxIgnoreAxis<' '>(minCorner, maxCorner, pos);
    }
    std::tuple<bool, float, block::BlockFace> getAABoxEnterFace(Vector3F minCorner,
                                                                Vector3F maxCorner) const
    {
        float minCornerX = collideWithPlane(Vector3F(-1, 0, 0), minCorner.x);
        float maxCornerX = collideWithPlane(Vector3F(-1, 0, 0), maxCorner.x);
        float xt = std::min(minCornerX, maxCornerX);
        auto xbf = minCornerX < maxCornerX ? block::BlockFace::NX : block::BlockFace::PX;
        float minCornerY = collideWithPlane(Vector3F(0, -1, 0), minCorner.y);
        float maxCornerY = collideWithPlane(Vector3F(0, -1, 0), maxCorner.y);
        float yt = std::min(minCornerY, maxCornerY);
        auto ybf = minCornerY < maxCornerY ? block::BlockFace::NY : block::BlockFace::PY;
        float minCornerZ = collideWithPlane(Vector3F(0, 0, -1), minCorner.z);
        float maxCornerZ = collideWithPlane(Vector3F(0, 0, -1), maxCorner.z);
        float zt = std::min(minCornerZ, maxCornerZ);
        auto zbf = minCornerZ < maxCornerZ ? block::BlockFace::NZ : block::BlockFace::PZ;
        if(xt > yt && xt > zt)
            return std::tuple<bool, float, block::BlockFace>(
                isPointInAABoxIgnoreAxis<'X'>(minCorner, maxCorner, eval(xt)), xt, xbf);
        else if(yt > zt)
            return std::tuple<bool, float, block::BlockFace>(
                isPointInAABoxIgnoreAxis<'Y'>(minCorner, maxCorner, eval(yt)), yt, ybf);
        else
            return std::tuple<bool, float, block::BlockFace>(
                isPointInAABoxIgnoreAxis<'Z'>(minCorner, maxCorner, eval(zt)), zt, zbf);
    }
    std::tuple<bool, float, block::BlockFace> getAABoxExitFace(Vector3F minCorner,
                                                               Vector3F maxCorner) const
    {
        float minCornerX = collideWithPlane(Vector3F(-1, 0, 0), minCorner.x);
        float maxCornerX = collideWithPlane(Vector3F(-1, 0, 0), maxCorner.x);
        float xt = std::max(minCornerX, maxCornerX);
        auto xbf = minCornerX > maxCornerX ? block::BlockFace::NX : block::BlockFace::PX;
        float minCornerY = collideWithPlane(Vector3F(0, -1, 0), minCorner.y);
        float maxCornerY = collideWithPlane(Vector3F(0, -1, 0), maxCorner.y);
        float yt = std::max(minCornerY, maxCornerY);
        auto ybf = minCornerY > maxCornerY ? block::BlockFace::NY : block::BlockFace::PY;
        float minCornerZ = collideWithPlane(Vector3F(0, 0, -1), minCorner.z);
        float maxCornerZ = collideWithPlane(Vector3F(0, 0, -1), maxCorner.z);
        float zt = std::max(minCornerZ, maxCornerZ);
        auto zbf = minCornerZ > maxCornerZ ? block::BlockFace::NZ : block::BlockFace::PZ;
        if(xt < yt && xt < zt && xt > 0)
            return std::tuple<bool, float, block::BlockFace>(
                isPointInAABoxIgnoreAxis<'X'>(minCorner, maxCorner, eval(xt)), xt, xbf);
        else if(yt < zt && yt > 0)
            return std::tuple<bool, float, block::BlockFace>(
                isPointInAABoxIgnoreAxis<'Y'>(minCorner, maxCorner, eval(yt)), yt, ybf);
        else
            return std::tuple<bool, float, block::BlockFace>(
                isPointInAABoxIgnoreAxis<'Z'>(minCorner, maxCorner, eval(zt)), zt, zbf);
    }
    friend Ray transform(const graphics::Transform &tform, Ray r)
    {
        return Ray(transform(tform, r.startPosition),
                   transformNormalUnnormalized(tform, r.direction));
    }
};

class RayBlockIterator
{
public:
    typedef std::pair<float, world::Position3I32> value_type;

private:
    Ray ray;
    std::pair<float, world::Position3I32> currentValue;
    Vector3F nextRayIntersectionTValues, rayIntersectionStepTValues;
    Vector3I32 positionDeltaValues;
    explicit RayBlockIterator(Ray ray);

public:
    RayBlockIterator() : RayBlockIterator(Ray())
    {
    }
    friend RayBlockIterator makeRayBlockIterator(Ray ray);
    const value_type &operator*() const
    {
        return currentValue;
    }
    const value_type *operator->() const
    {
        return &currentValue;
    }
    RayBlockIterator operator++(int)
    {
        RayBlockIterator retval = *this;
        operator++();
        return retval;
    }
    const RayBlockIterator &operator++();
};

inline RayBlockIterator makeRayBlockIterator(Ray ray)
{
    return RayBlockIterator(ray);
}

struct Collision final
{
    float t;
    enum class Type : std::uint8_t
    {
        None,
        Block,
        Entity,
        DEFINE_ENUM_LIMITS(None, Entity)
    };
    Type type;
    world::World *world;
    world::Position3I32 blockPosition;
    Optional<block::BlockFace> blockFace;
    entity::Entity *entity;
    constexpr Collision()
        : t(-1), type(Type::None), world(nullptr), blockPosition(), blockFace(), entity(nullptr)
    {
    }
    constexpr explicit Collision(world::World &world)
        : t(-1), type(Type::None), world(&world), blockPosition(), blockFace(), entity(nullptr)
    {
    }
    Collision(world::World &world,
              float t,
              world::Position3I32 blockPosition,
              Optional<block::BlockFace> blockFace)
        : t(t),
          type(Type::Block),
          world(&world),
          blockPosition(blockPosition),
          blockFace(blockFace),
          entity(nullptr)
    {
    }
    constexpr Collision(world::World &world, float t, entity::Entity &entity)
        : t(t), type(Type::Entity), world(&world), blockPosition(), blockFace(), entity(&entity)
    {
    }
    constexpr bool valid() const
    {
        return t >= Ray::eps && world != nullptr && ((type == Type::Entity && entity != nullptr)
                                                     || (type == Type::Block && entity == nullptr));
    }
    constexpr bool operator<(const Collision &r) const
    {
        return valid() && (!r.valid() || t < r.t);
    }
    constexpr bool operator==(const Collision &r) const
    {
        return type == r.type
               && (type == Type::None || (t < Ray::eps && r.t < Ray::eps) || t == r.t)
               && (type != Type::Block || blockPosition == r.blockPosition)
               && (type != Type::Entity || entity == r.entity);
    }
    constexpr bool operator!=(const Collision &r) const
    {
        return !operator==(r);
    }
    constexpr bool operator>(const Collision &r) const
    {
        return r < *this;
    }
    constexpr bool operator>=(const Collision &r) const
    {
        return !operator<(r);
    }
    constexpr bool operator<=(const Collision &r) const
    {
        return !(r < *this);
    }
};
}
}
}
}

#endif // RAY_CASTING_H_INCLUDED
