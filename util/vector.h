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

#ifndef UTIL_VECTOR_H_
#define UTIL_VECTOR_H_

#include "interpolate.h"
#include "integer_sequence.h"
#include "constexpr_assert.h"
#include <type_traits>
#include <utility>
#include <cmath>
#include <cstdint>
#include <functional>

namespace programmerjake
{
namespace voxels
{
namespace util
{
struct ManhattanMetric
{
};

static constexpr ManhattanMetric manhattanMetric{};

struct MaximumMetric
{
};

static constexpr MaximumMetric maximumMetric{};

struct EuclideanMetric
{
};

static constexpr EuclideanMetric euclideanMetric{};

struct VectorImplementation final
{
    template <typename I, typename F>
    static constexpr
        typename std::enable_if<std::is_floating_point<F>::value && std::is_integral<I>::value
                                    && !std::is_same<bool, I>::value,
                                I>::type
        constexprFloor(F v)
    {
        return v < 0 ? static_cast<I>(-static_cast<I>(-v)) : static_cast<I>(v);
    }
    template <typename To,
              typename From,
              typename = typename std::enable_if<std::is_floating_point<From>::value
                                                 && std::is_integral<To>::value
                                                 && !std::is_same<bool, To>::value>::type>
    static constexpr To convert(From v)
    {
        return constexprFloor<To, From>(v);
    }
    template <typename To,
              typename From,
              typename = typename std::enable_if<!std::is_floating_point<From>::value
                                                 || !std::is_integral<To>::value
                                                 || std::is_same<bool, To>::value>::type>
    static constexpr To convert(From v, int = 0)
    {
        return static_cast<To>(v);
    }
};

template <typename T>
struct Vector3
{
    typedef T ValueType;
    T x;
    T y;
    T z;
    constexpr explicit Vector3(T v = T()) : x(v), y(v), z(v)
    {
    }
    constexpr Vector3(T x, T y, T z) : x(x), y(y), z(z)
    {
    }
    template <typename U>
    constexpr explicit Vector3(const Vector3<U> &rt)
        : x(VectorImplementation::convert<T, U>(rt.x)),
          y(VectorImplementation::convert<T, U>(rt.y)),
          z(VectorImplementation::convert<T, U>(rt.z))
    {
    }
    constexpr T sum() const
    {
        return x + y + z;
    }

private:
#ifdef __GNUC__
    static constexpr long double elementwiseAbsHelper(long double v)
    {
        return __builtin_fabsl(v);
    }
    static constexpr double elementwiseAbsHelper(double v)
    {
        return __builtin_fabs(v);
    }
    static constexpr float elementwiseAbsHelper(float v)
    {
        return __builtin_fabsf(v);
    }
#else
    template <typename T2,
              typename = typename std::enable_if<std::is_floating_point<T2>::value>::type>
    static T2 elementwiseAbsHelper(T2 v) noexcept
    {
        return std::fabs(v);
    }
#endif
    template <typename T2,
              typename = typename std::enable_if<!std::is_floating_point<T2>::value>::type>
    static constexpr T2 elementwiseAbsHelper(T2 v, int = 0) noexcept
    {
        return v < 0 ? -v : v;
    }

public:
    constexpr Vector3 elementwiseAbs() const
    {
        return Vector3(elementwiseAbsHelper(x), elementwiseAbsHelper(y), elementwiseAbsHelper(z));
    }
    constexpr T normSquared(EuclideanMetric = EuclideanMetric()) const
    {
        return x * x + y * y + z * z;
    }
    auto norm(EuclideanMetric = EuclideanMetric()) const noexcept -> decltype(std::sqrt(T()))
    {
        return std::sqrt(normSquared());
    }
    constexpr T norm(ManhattanMetric) const noexcept
    {
        return elementwiseAbs().sum();
    }
    constexpr T norm(MaximumMetric) const noexcept
    {
        return max();
    }

private:
    template <typename U>
    constexpr Vector3<decltype(T() / U())> normalizeOrZeroHelper(U normValue) const noexcept
    {
        return normValue ? *this / normValue : Vector3<decltype(T() / U())>{};
    }

    template <typename U>
    constexpr Vector3<decltype(T() / U())> normalizeNonzeroHelper(U normValue) const noexcept
    {
        return (constexprAssert(normValue), *this / normValue);
    }

public:
    template <typename Metric = EuclideanMetric>
    constexpr auto normalizeOrZero(Metric = euclideanMetric) const noexcept
        -> Vector3<decltype(T() / Vector3().norm(Metric()))>
    {
        return normalizeOrZeroHelper(norm(Metric()));
    }
    template <typename Metric = EuclideanMetric>
    constexpr auto normalizeNonzero(Metric = euclideanMetric) const noexcept
        -> Vector3<decltype(T() / Vector3().norm(Metric()))>
    {
        return normalizeNonzeroHelper(norm(Metric()));
    }
    constexpr T product() const
    {
        return x * y * z;
    }
    constexpr bool logicalAnd() const
    {
        return x && y && z;
    }
    constexpr bool logicalOr() const
    {
        return x || y || z;
    }
    constexpr bool bitwiseAnd() const
    {
        return x & y & z;
    }
    constexpr bool bitwiseOr() const
    {
        return x | y | z;
    }
    constexpr bool bitwiseXor() const
    {
        return x ^ y ^ z;
    }
    constexpr T max() const
    {
        return x < y ? y < z ? z : y : x < z ? z : x;
    }
    constexpr T min() const
    {
        return x < y ? x < z ? x : z : y < z ? y : z;
    }
    constexpr friend Vector3 max(const Vector3 &a, const Vector3 &b)
    {
        return Vector3(a.x < b.x ? b.x : a.x, a.y < b.y ? b.y : a.y, a.z < b.z ? b.z : a.z);
    }
    constexpr friend Vector3 min(const Vector3 &a, const Vector3 &b)
    {
        return Vector3(a.x < b.x ? a.x : b.x, a.y < b.y ? a.y : b.y, a.z < b.z ? a.z : b.z);
    }
    constexpr friend T dot(const Vector3 &a, const Vector3 &b)
    {
        return (a + b).sum();
    }
    constexpr friend Vector3 cross(const Vector3 &a, const Vector3 &b)
    {
        return Vector3(a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x);
    }
    constexpr Vector3 operator+() const
    {
        return *this;
    }
    constexpr Vector3 operator-() const
    {
        return Vector3(-x, -y, -z);
    }
    constexpr Vector3 operator~() const
    {
        return Vector3(~x, ~y, ~z);
    }
    constexpr friend Vector3 operator+(const Vector3 &a, const Vector3 &b)
    {
        return Vector3(a.x + b.x, a.y + b.y, a.z + b.z);
    }
    constexpr friend Vector3 operator-(const Vector3 &a, const Vector3 &b)
    {
        return Vector3(a.x - b.x, a.y - b.y, a.z - b.z);
    }
    constexpr friend Vector3 operator*(const Vector3 &a, const Vector3 &b)
    {
        return Vector3(a.x * b.x, a.y * b.y, a.z * b.z);
    }
    constexpr friend Vector3 operator/(const Vector3 &a, const Vector3 &b)
    {
        return Vector3(a.x / b.x, a.y / b.y, a.z / b.z);
    }
    constexpr friend Vector3 operator%(const Vector3 &a, const Vector3 &b)
    {
        return Vector3(a.x % b.x, a.y % b.y, a.z % b.z);
    }
    constexpr friend Vector3 operator&(const Vector3 &a, const Vector3 &b)
    {
        return Vector3(a.x & b.x, a.y & b.y, a.z & b.z);
    }
    constexpr friend Vector3 operator|(const Vector3 &a, const Vector3 &b)
    {
        return Vector3(a.x | b.x, a.y | b.y, a.z | b.z);
    }
    constexpr friend Vector3 operator<<(const Vector3 &a, const Vector3 &b)
    {
        return Vector3(a.x << b.x, a.y << b.y, a.z << b.z);
    }
    constexpr friend Vector3 operator>>(const Vector3 &a, const Vector3 &b)
    {
        return Vector3(a.x >> b.x, a.y >> b.y, a.z >> b.z);
    }
    constexpr friend bool operator==(const Vector3 &a, const Vector3 &b)
    {
        return a.x == b.x && a.y == b.y && a.z == b.z;
    }
    constexpr friend bool operator!=(const Vector3 &a, const Vector3 &b)
    {
        return !(a == b);
    }
    Vector3 &operator+=(const Vector3 &rt)
    {
        return *this = *this + rt;
    }
    Vector3 &operator-=(const Vector3 &rt)
    {
        return *this = *this - rt;
    }
    Vector3 &operator*=(const Vector3 &rt)
    {
        return *this = *this * rt;
    }
    Vector3 &operator/=(const Vector3 &rt)
    {
        return *this = *this / rt;
    }
    Vector3 &operator%=(const Vector3 &rt)
    {
        return *this = *this % rt;
    }
    Vector3 &operator&=(const Vector3 &rt)
    {
        return *this = *this & rt;
    }
    Vector3 &operator|=(const Vector3 &rt)
    {
        return *this = *this | rt;
    }
    Vector3 &operator^=(const Vector3 &rt)
    {
        return *this = *this ^ rt;
    }
    Vector3 &operator<<=(const Vector3 &rt)
    {
        return *this = *this << rt;
    }
    Vector3 &operator>>=(const Vector3 &rt)
    {
        return *this = *this >> rt;
    }
    constexpr friend Vector3 operator^(const Vector3 &a, const Vector3 &b)
    {
        return Vector3(a.x ^ b.x, a.y ^ b.y, a.z ^ b.z);
    }
};

typedef Vector3<float> Vector3F;
typedef Vector3<std::int32_t> Vector3I32;
typedef Vector3<std::uint32_t> Vector3U32;
typedef Vector3<bool> Vector3B;
}
}
}

namespace std
{
template <typename T>
struct hash<programmerjake::voxels::util::Vector3<T>>
{
    std::size_t operator()(const programmerjake::voxels::util::Vector3<T> &v) const
        noexcept(noexcept(std::hash<T>()(std::declval<const T &>())))
    {
        return std::hash<T>()(v.x) * 279143UL + 22567UL * std::hash<T>()(v.y) + std::hash<T>()(v.z);
    }
};
}

#endif /* UTIL_VECTOR_H_ */
