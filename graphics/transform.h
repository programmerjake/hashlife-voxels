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

#ifndef GRAPHICS_TRANSFORM_H_
#define GRAPHICS_TRANSFORM_H_

#include "../util/vector.h"
#include "../util/matrix.h"

namespace programmerjake
{
namespace voxels
{
namespace graphics
{
struct Transform final
{
    util::Matrix4x4F positionMatrix;
    util::Matrix4x4F normalMatrix;
    constexpr explicit Transform(const util::Matrix4x4F &matrix) noexcept
        : positionMatrix(matrix),
          normalMatrix(inverse(transpose(matrix)))
    {
    }
    constexpr Transform(const util::Matrix4x4F &positionMatrix,
                        const util::Matrix4x4F &normalMatrix) noexcept
        : positionMatrix(positionMatrix),
          normalMatrix(normalMatrix)
    {
    }
    constexpr Transform() noexcept : positionMatrix(util::Matrix4x4F::identity()),
                                     normalMatrix(util::Matrix4x4F::identity())
    {
    }
    constexpr Transform concat(const Transform &rt) const noexcept
    {
        return Transform(positionMatrix.concat(rt.positionMatrix),
                         normalMatrix.concat(rt.normalMatrix));
    }
    friend constexpr Transform inverse(const Transform &transform) noexcept
    {
        return Transform(transpose(transform.normalMatrix), transpose(transform.positionMatrix));
    }
    friend constexpr Transform transpose(const Transform &transform) noexcept
    {
        return Transform(transpose(transform.positionMatrix), transpose(transform.normalMatrix));
    }
    static constexpr Transform identity() noexcept
    {
        return Transform(util::Matrix4x4F::identity(), util::Matrix4x4F::identity());
    }
    static constexpr Transform scale(float v) noexcept
    {
        return Transform(util::Matrix4x4F::scale(v), util::Matrix4x4F::scale(1.0f / v));
    }
    static constexpr Transform scale(float x, float y, float z) noexcept
    {
        return Transform(util::Matrix4x4F::scale(x, y, z),
                         util::Matrix4x4F::scale(1.0f / x, 1.0f / y, 1.0f / z));
    }
    static constexpr Transform scale(util::Vector3F v) noexcept
    {
        return scale(v.x, v.y, v.z);
    }
    static constexpr Transform translate(util::Vector3F v) noexcept
    {
        return Transform(util::Matrix4x4F::translate(v),
                         transpose(util::Matrix4x4F::translate(-v)));
    }
    static constexpr Transform translate(float x, float y, float z) noexcept
    {
        return translate(util::Vector3F(x, y, z));
    }
    static Transform rotate(util::Vector3F axis, double angle) noexcept
    {
        auto matrix = util::Matrix4x4F::rotate(axis, angle);
        return Transform(matrix, matrix);
    }
    static Transform rotateX(double angle) noexcept
    {
        auto matrix = util::Matrix4x4F::rotateX(angle);
        return Transform(matrix, matrix);
    }
    static Transform rotateY(double angle) noexcept
    {
        auto matrix = util::Matrix4x4F::rotateY(angle);
        return Transform(matrix, matrix);
    }
    static Transform rotateZ(double angle) noexcept
    {
        auto matrix = util::Matrix4x4F::rotateZ(angle);
        return Transform(matrix, matrix);
    }
    static constexpr Transform frustum(
        float left, float right, float bottom, float top, float front, float back) noexcept
    {
        return Transform(
            util::Matrix4x4F::frustum(left, right, bottom, top, front, back),
            util::Matrix4x4F::inverseTransposeFrustum(left, right, bottom, top, front, back));
    }
    static constexpr Transform ortho(
        float left, float right, float bottom, float top, float front, float back) noexcept
    {
        return Transform(
            util::Matrix4x4F::ortho(left, right, bottom, top, front, back),
            util::Matrix4x4F::inverseTransposeOrtho(left, right, bottom, top, front, back));
    }
    constexpr bool operator==(const Transform &rt) const noexcept
    {
        return positionMatrix == rt.positionMatrix && normalMatrix == rt.normalMatrix;
    }
    constexpr bool operator!=(const Transform &rt) const noexcept
    {
        return !operator==(rt);
    }
    friend constexpr util::Vector3F transform(const Transform &transform, util::Vector3F v) noexcept
    {
        return transform.positionMatrix.apply(v);
    }
    friend constexpr util::Vector3F transformNormal(const Transform &transform,
                                                    util::Vector3F v) noexcept
    {
        return transform.normalMatrix.apply(v).normalizeOrZero();
    }
    friend constexpr util::Matrix4x4F transform(const Transform &a,
                                                const util::Matrix4x4F &b) noexcept
    {
        return b.concat(a.positionMatrix);
    }
    friend constexpr Transform transform(const Transform &a, const Transform &b) noexcept
    {
        return b.concat(a);
    }
    constexpr explicit operator util::Matrix4x4F() const noexcept
    {
        return positionMatrix;
    }
};
}
}
}

#endif /* GRAPHICS_TRANSFORM_H_ */
