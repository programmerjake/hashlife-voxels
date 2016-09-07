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

#ifndef UTIL_MATRIX_H_
#define UTIL_MATRIX_H_

#include "constexpr_array.h"
#include "vector.h"

namespace programmerjake
{
namespace voxels
{
namespace util
{
struct Matrix4x4F final
{
    Array<Array<float, 4>, 4> elements;
    Array<float, 4> &operator[](std::size_t index) noexcept
    {
        return elements[index];
    }
    constexpr const Array<float, 4> &operator[](std::size_t index) const noexcept
    {
        return elements[index];
    }
    constexpr Matrix4x4F(float x00,
                         float x10,
                         float x20,
                         float x30,
                         float x01,
                         float x11,
                         float x21,
                         float x31,
                         float x02,
                         float x12,
                         float x22,
                         float x32,
                         float x03,
                         float x13,
                         float x23,
                         float x33) noexcept : elements{{{{x00, x01, x02, x03}},
                                                         {{x10, x11, x12, x13}},
                                                         {{x20, x21, x22, x23}},
                                                         {{x30, x31, x32, x33}}}}
    {
    }
    constexpr Matrix4x4F(float x00,
                         float x10,
                         float x20,
                         float x30,
                         float x01,
                         float x11,
                         float x21,
                         float x31,
                         float x02,
                         float x12,
                         float x22,
                         float x32) noexcept : elements{{{{x00, x01, x02, 0}},
                                                         {{x10, x11, x12, 0}},
                                                         {{x20, x21, x22, 0}},
                                                         {{x30, x31, x32, 1}}}}
    {
    }
    constexpr Matrix4x4F() noexcept : elements{{{{1, 0, 0, 0}}, // identity matrix
                                                {{0, 1, 0, 0}},
                                                {{0, 0, 1, 0}},
                                                {{0, 0, 0, 1}}}}

    {
    }
    constexpr static Matrix4x4F identity() noexcept
    {
        return Matrix4x4F();
    }
    static Matrix4x4F rotate(Vector3F axis, double angle) noexcept
    {
        auto axisv = axis.normalizeOrZero();
        auto c = static_cast<float>(std::cos(angle));
        auto s = static_cast<float>(std::sin(angle));
        auto v = 1 - c; // Versine
        auto xx = axisv.x * axisv.x;
        auto xy = axisv.x * axisv.y;
        auto xz = axisv.x * axisv.z;
        auto yy = axisv.y * axisv.y;
        auto yz = axisv.y * axisv.z;
        auto zz = axisv.z * axisv.z;
        return Matrix4x4F(xx + (1 - xx) * c,
                          xy * v - axisv.z * s,
                          xz * v + axisv.y * s,
                          0,
                          xy * v + axisv.z * s,
                          yy + (1 - yy) * c,
                          yz * v - axisv.x * s,
                          0,
                          xz * v - axisv.y * s,
                          yz * v + axisv.x * s,
                          zz + (1 - zz) * c,
                          0);
    }
    static Matrix4x4F rotateX(double angle) noexcept
    {
        auto c = static_cast<float>(std::cos(angle));
        auto s = static_cast<float>(std::sin(angle));
        return Matrix4x4F(1, 0, 0, 0, 0, c, -s, 0, 0, s, c, 0);
    }
    static Matrix4x4F rotateY(double angle) noexcept
    {
        auto c = static_cast<float>(std::cos(angle));
        auto s = static_cast<float>(std::sin(angle));
        return Matrix4x4F(c, 0, s, 0, 0, 1, 0, 0, -s, 0, c, 0);
    }
    static Matrix4x4F rotateZ(double angle) noexcept
    {
        auto c = static_cast<float>(std::cos(angle));
        auto s = static_cast<float>(std::sin(angle));
        return Matrix4x4F(c, -s, 0, 0, s, c, 0, 0, 0, 0, 1, 0);
    }
    constexpr static Matrix4x4F translate(Vector3F position) noexcept
    {
        return Matrix4x4F(1, 0, 0, position.x, 0, 1, 0, position.y, 0, 0, 1, position.z);
    }
    constexpr static Matrix4x4F translate(float x, float y, float z) noexcept
    {
        return Matrix4x4F(1, 0, 0, x, 0, 1, 0, y, 0, 0, 1, z);
    }
    constexpr static Matrix4x4F scale(float x, float y, float z) noexcept
    {
        return Matrix4x4F(x, 0, 0, 0, 0, y, 0, 0, 0, 0, z, 0);
    }
    constexpr static Matrix4x4F scale(Vector3F s) noexcept
    {
        return Matrix4x4F(s.x, 0, 0, 0, 0, s.y, 0, 0, 0, 0, s.z, 0);
    }
    constexpr static Matrix4x4F scale(float s) noexcept
    {
        return Matrix4x4F(s, 0, 0, 0, 0, s, 0, 0, 0, 0, s, 0);
    }
    static constexpr Matrix4x4F frustum(
        float left, float right, float bottom, float top, float front, float back) noexcept
    {
        return Matrix4x4F(2 * front / (right - left),
                          0,
                          (right + left) / (right - left),
                          0,
                          0,
                          2 * front / (top - bottom),
                          (top + bottom) / (top - bottom),
                          0,
                          0,
                          0,
                          (back + front) / (front - back),
                          2 * front * back / (front - back),
                          0,
                          0,
                          -1,
                          0);
    }
    static constexpr Matrix4x4F inverseTransposeFrustum(
        float left, float right, float bottom, float top, float front, float back) noexcept
    {
        return Matrix4x4F((right - left) / (2 * front),
                          0,
                          0,
                          0,
                          0,
                          (top - bottom) / (2 * front),
                          0,
                          0,
                          0,
                          0,
                          0,
                          (front - back) / (2 * front * back),
                          (left + right) / (2 * front),
                          (bottom + top) / (2 * front),
                          -1,
                          (back + front) / (2 * front * back));
    }
    static constexpr Matrix4x4F ortho(
        float left, float right, float bottom, float top, float front, float back) noexcept
    {
        return Matrix4x4F(-2 / (left - right),
                          0,
                          0,
                          (right + left) / (left - right),

                          0,
                          -2 / (bottom - top),
                          0,
                          (top + bottom) / (bottom - top),

                          0,
                          0,
                          2 / (front - back),
                          (back + front) / (front - back));
    }
    static constexpr Matrix4x4F inverseTransposeOrtho(
        float left, float right, float bottom, float top, float front, float back) noexcept
    {
        return Matrix4x4F((right - left) * 0.5f,
                          0,
                          0,
                          0,

                          0,
                          (top - bottom) * 0.5f,
                          0,
                          0,

                          0,
                          0,
                          (front - back) * 0.5f,
                          0,

                          (left + right) * 0.5f,
                          (bottom + top) * 0.5f,
                          (front + back) * -0.5f,
                          1);
    }
    constexpr float determinant() const noexcept
    {
        return elements[0][3] * elements[1][2] * elements[2][1] * elements[3][0]
               - elements[0][2] * elements[1][3] * elements[2][1] * elements[3][0]
               - elements[0][3] * elements[1][1] * elements[2][2] * elements[3][0]
               + elements[0][1] * elements[1][3] * elements[2][2] * elements[3][0]
               + elements[0][2] * elements[1][1] * elements[2][3] * elements[3][0]
               - elements[0][1] * elements[1][2] * elements[2][3] * elements[3][0]
               - elements[0][3] * elements[1][2] * elements[2][0] * elements[3][1]
               + elements[0][2] * elements[1][3] * elements[2][0] * elements[3][1]
               + elements[0][3] * elements[1][0] * elements[2][2] * elements[3][1]
               - elements[0][0] * elements[1][3] * elements[2][2] * elements[3][1]
               - elements[0][2] * elements[1][0] * elements[2][3] * elements[3][1]
               + elements[0][0] * elements[1][2] * elements[2][3] * elements[3][1]
               + elements[0][3] * elements[1][1] * elements[2][0] * elements[3][2]
               - elements[0][1] * elements[1][3] * elements[2][0] * elements[3][2]
               - elements[0][3] * elements[1][0] * elements[2][1] * elements[3][2]
               + elements[0][0] * elements[1][3] * elements[2][1] * elements[3][2]
               + elements[0][1] * elements[1][0] * elements[2][3] * elements[3][2]
               - elements[0][0] * elements[1][1] * elements[2][3] * elements[3][2]
               - elements[0][2] * elements[1][1] * elements[2][0] * elements[3][3]
               + elements[0][1] * elements[1][2] * elements[2][0] * elements[3][3]
               + elements[0][2] * elements[1][0] * elements[2][1] * elements[3][3]
               - elements[0][0] * elements[1][2] * elements[2][1] * elements[3][3]
               - elements[0][1] * elements[1][0] * elements[2][2] * elements[3][3]
               + elements[0][0] * elements[1][1] * elements[2][2] * elements[3][3];
    }

private:
    constexpr Matrix4x4F inverseHelper2(float factor) const noexcept
    {
        return Matrix4x4F(
            factor * ((elements[1][1] * elements[2][2] - elements[2][1] * elements[1][2])
                          * elements[3][3]
                      + (elements[3][1] * elements[1][2] - elements[1][1] * elements[3][2])
                            * elements[2][3]
                      + (elements[2][1] * elements[3][2] - elements[3][1] * elements[2][2])
                            * elements[1][3]),
            factor * ((elements[2][0] * elements[1][2] - elements[1][0] * elements[2][2])
                          * elements[3][3]
                      + (elements[1][0] * elements[3][2] - elements[3][0] * elements[1][2])
                            * elements[2][3]
                      + (elements[3][0] * elements[2][2] - elements[2][0] * elements[3][2])
                            * elements[1][3]),
            factor * ((elements[1][0] * elements[2][1] - elements[2][0] * elements[1][1])
                          * elements[3][3]
                      + (elements[3][0] * elements[1][1] - elements[1][0] * elements[3][1])
                            * elements[2][3]
                      + (elements[2][0] * elements[3][1] - elements[3][0] * elements[2][1])
                            * elements[1][3]),
            factor * ((elements[2][0] * elements[1][1] - elements[1][0] * elements[2][1])
                          * elements[3][2]
                      + (elements[1][0] * elements[3][1] - elements[3][0] * elements[1][1])
                            * elements[2][2]
                      + (elements[3][0] * elements[2][1] - elements[2][0] * elements[3][1])
                            * elements[1][2]),
            factor * ((elements[2][1] * elements[0][2] - elements[0][1] * elements[2][2])
                          * elements[3][3]
                      + (elements[0][1] * elements[3][2] - elements[3][1] * elements[0][2])
                            * elements[2][3]
                      + (elements[3][1] * elements[2][2] - elements[2][1] * elements[3][2])
                            * elements[0][3]),
            factor * ((elements[0][0] * elements[2][2] - elements[2][0] * elements[0][2])
                          * elements[3][3]
                      + (elements[3][0] * elements[0][2] - elements[0][0] * elements[3][2])
                            * elements[2][3]
                      + (elements[2][0] * elements[3][2] - elements[3][0] * elements[2][2])
                            * elements[0][3]),
            factor * ((elements[2][0] * elements[0][1] - elements[0][0] * elements[2][1])
                          * elements[3][3]
                      + (elements[0][0] * elements[3][1] - elements[3][0] * elements[0][1])
                            * elements[2][3]
                      + (elements[3][0] * elements[2][1] - elements[2][0] * elements[3][1])
                            * elements[0][3]),
            factor * ((elements[0][0] * elements[2][1] - elements[2][0] * elements[0][1])
                          * elements[3][2]
                      + (elements[3][0] * elements[0][1] - elements[0][0] * elements[3][1])
                            * elements[2][2]
                      + (elements[2][0] * elements[3][1] - elements[3][0] * elements[2][1])
                            * elements[0][2]),
            factor * ((elements[0][1] * elements[1][2] - elements[1][1] * elements[0][2])
                          * elements[3][3]
                      + (elements[3][1] * elements[0][2] - elements[0][1] * elements[3][2])
                            * elements[1][3]
                      + (elements[1][1] * elements[3][2] - elements[3][1] * elements[1][2])
                            * elements[0][3]),
            factor * ((elements[1][0] * elements[0][2] - elements[0][0] * elements[1][2])
                          * elements[3][3]
                      + (elements[0][0] * elements[3][2] - elements[3][0] * elements[0][2])
                            * elements[1][3]
                      + (elements[3][0] * elements[1][2] - elements[1][0] * elements[3][2])
                            * elements[0][3]),
            factor * ((elements[0][0] * elements[1][1] - elements[1][0] * elements[0][1])
                          * elements[3][3]
                      + (elements[3][0] * elements[0][1] - elements[0][0] * elements[3][1])
                            * elements[1][3]
                      + (elements[1][0] * elements[3][1] - elements[3][0] * elements[1][1])
                            * elements[0][3]),
            factor * ((elements[1][0] * elements[0][1] - elements[0][0] * elements[1][1])
                          * elements[3][2]
                      + (elements[0][0] * elements[3][1] - elements[3][0] * elements[0][1])
                            * elements[1][2]
                      + (elements[3][0] * elements[1][1] - elements[1][0] * elements[3][1])
                            * elements[0][2]),
            factor * ((elements[1][1] * elements[0][2] - elements[0][1] * elements[1][2])
                          * elements[2][3]
                      + (elements[0][1] * elements[2][2] - elements[2][1] * elements[0][2])
                            * elements[1][3]
                      + (elements[2][1] * elements[1][2] - elements[1][1] * elements[2][2])
                            * elements[0][3]),
            factor * ((elements[0][0] * elements[1][2] - elements[1][0] * elements[0][2])
                          * elements[2][3]
                      + (elements[2][0] * elements[0][2] - elements[0][0] * elements[2][2])
                            * elements[1][3]
                      + (elements[1][0] * elements[2][2] - elements[2][0] * elements[1][2])
                            * elements[0][3]),
            factor * ((elements[1][0] * elements[0][1] - elements[0][0] * elements[1][1])
                          * elements[2][3]
                      + (elements[0][0] * elements[2][1] - elements[2][0] * elements[0][1])
                            * elements[1][3]
                      + (elements[2][0] * elements[1][1] - elements[1][0] * elements[2][1])
                            * elements[0][3]),
            factor * ((elements[0][0] * elements[1][1] - elements[1][0] * elements[0][1])
                          * elements[2][2]
                      + (elements[2][0] * elements[0][1] - elements[0][0] * elements[2][1])
                            * elements[1][2]
                      + (elements[1][0] * elements[2][1] - elements[2][0] * elements[1][1])
                            * elements[0][2]));
    }
    constexpr Matrix4x4F inverseHelper(float det) const noexcept
    {
        return inverseHelper2(1.0f / det);
    }

public:
    friend constexpr Matrix4x4F inverse(const Matrix4x4F &v) noexcept
    {
        return v.inverseHelper(v.determinant());
    }
    friend constexpr Matrix4x4F transpose(const Matrix4x4F &v) noexcept
    {
        return Matrix4x4F(v[0][0],
                          v[0][1],
                          v[0][2],
                          v[0][3],
                          v[1][0],
                          v[1][1],
                          v[1][2],
                          v[1][3],
                          v[2][0],
                          v[2][1],
                          v[2][2],
                          v[2][3],
                          v[3][0],
                          v[3][1],
                          v[3][2],
                          v[3][3]);
    }
    constexpr Matrix4x4F concat(const Matrix4x4F rt) const noexcept
    {
        return Matrix4x4F(/* elements[0][0] */ elements[0][0] * rt[0][0] + elements[0][1] * rt[1][0]
                              + elements[0][2] * rt[2][0]
                              + elements[0][3] * rt[3][0],
                          /* elements[1][0] */ elements[1][0] * rt[0][0] + elements[1][1] * rt[1][0]
                              + elements[1][2] * rt[2][0]
                              + elements[1][3] * rt[3][0],
                          /* elements[2][0] */ elements[2][0] * rt[0][0] + elements[2][1] * rt[1][0]
                              + elements[2][2] * rt[2][0]
                              + elements[2][3] * rt[3][0],
                          /* elements[3][0] */ elements[3][0] * rt[0][0] + elements[3][1] * rt[1][0]
                              + elements[3][2] * rt[2][0]
                              + elements[3][3] * rt[3][0],
                          /* elements[0][1] */ elements[0][0] * rt[0][1] + elements[0][1] * rt[1][1]
                              + elements[0][2] * rt[2][1]
                              + elements[0][3] * rt[3][1],
                          /* elements[1][1] */ elements[1][0] * rt[0][1] + elements[1][1] * rt[1][1]
                              + elements[1][2] * rt[2][1]
                              + elements[1][3] * rt[3][1],
                          /* elements[2][1] */ elements[2][0] * rt[0][1] + elements[2][1] * rt[1][1]
                              + elements[2][2] * rt[2][1]
                              + elements[2][3] * rt[3][1],
                          /* elements[3][1] */ elements[3][0] * rt[0][1] + elements[3][1] * rt[1][1]
                              + elements[3][2] * rt[2][1]
                              + elements[3][3] * rt[3][1],
                          /* elements[0][2] */ elements[0][0] * rt[0][2] + elements[0][1] * rt[1][2]
                              + elements[0][2] * rt[2][2]
                              + elements[0][3] * rt[3][2],
                          /* elements[1][2] */ elements[1][0] * rt[0][2] + elements[1][1] * rt[1][2]
                              + elements[1][2] * rt[2][2]
                              + elements[1][3] * rt[3][2],
                          /* elements[2][2] */ elements[2][0] * rt[0][2] + elements[2][1] * rt[1][2]
                              + elements[2][2] * rt[2][2]
                              + elements[2][3] * rt[3][2],
                          /* elements[3][2] */ elements[3][0] * rt[0][2] + elements[3][1] * rt[1][2]
                              + elements[3][2] * rt[2][2]
                              + elements[3][3] * rt[3][2],
                          /* elements[0][3] */ elements[0][0] * rt[0][3] + elements[0][1] * rt[1][3]
                              + elements[0][2] * rt[2][3]
                              + elements[0][3] * rt[3][3],
                          /* elements[1][3] */ elements[1][0] * rt[0][3] + elements[1][1] * rt[1][3]
                              + elements[1][2] * rt[2][3]
                              + elements[1][3] * rt[3][3],
                          /* elements[2][3] */ elements[2][0] * rt[0][3] + elements[2][1] * rt[1][3]
                              + elements[2][2] * rt[2][3]
                              + elements[2][3] * rt[3][3],
                          /* elements[3][3] */ elements[3][0] * rt[0][3] + elements[3][1] * rt[1][3]
                              + elements[3][2] * rt[2][3]
                              + elements[3][3] * rt[3][3]);
    }

private:
    static constexpr Vector3F applyHelper(Vector3F retval, float divisor) noexcept
    {
        return divisor == 1 ? retval : retval / Vector3F(divisor);
    }

public:
    constexpr Vector3F apply(Vector3F v) const noexcept
    {
        return applyHelper(
            Vector3F{
                v.x * elements[0][0] + v.y * elements[1][0] + v.z * elements[2][0] + elements[3][0],
                v.x * elements[0][1] + v.y * elements[1][1] + v.z * elements[2][1] + elements[3][1],
                v.x * elements[0][2] + v.y * elements[1][2] + v.z * elements[2][2] + elements[3][2],
            },
            v.x * elements[0][3] + v.y * elements[1][3] + v.z * elements[2][3] + elements[3][3]);
    }
    constexpr Vector3F applyNoTranslate(Vector3F v) const noexcept
    {
        return Vector3F(v.x * elements[0][0] + v.y * elements[1][0] + v.z * elements[2][0],
                        v.x * elements[0][1] + v.y * elements[1][1] + v.z * elements[2][1],
                        v.x * elements[0][2] + v.y * elements[1][2] + v.z * elements[2][2]);
    }
    constexpr bool operator==(const Matrix4x4F &rt) const noexcept
    {
        return elements[0][0] == rt[0][0] && elements[0][1] == rt[0][1]
               && elements[0][2] == rt[0][2] && elements[0][3] == rt[0][3]
               && elements[1][0] == rt[1][0] && elements[1][1] == rt[1][1]
               && elements[1][2] == rt[1][2] && elements[1][3] == rt[1][3]
               && elements[2][0] == rt[2][0] && elements[2][1] == rt[2][1]
               && elements[2][2] == rt[2][2] && elements[2][3] == rt[2][3]
               && elements[3][0] == rt[3][0] && elements[3][1] == rt[3][1]
               && elements[3][2] == rt[3][2] && elements[3][3] == rt[3][3];
    }
    constexpr bool operator!=(const Matrix4x4F &rt) const noexcept
    {
        return !operator==(rt);
    }
};
}
}
}

#endif /* UTIL_MATRIX_H_ */
