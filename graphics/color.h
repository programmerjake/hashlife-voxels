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

#ifndef GRAPHICS_COLOR_H_
#define GRAPHICS_COLOR_H_

#include <cstdint>
#include <type_traits>
#include <limits>
#include "../util/constexpr_assert.h"
#include "../util/interpolate.h"

namespace programmerjake
{
namespace voxels
{
namespace graphics
{
template <typename T,
          bool isUnsignedInteger = (std::is_arithmetic<T>::value && std::is_unsigned<T>::value
                                    && (std::numeric_limits<T>::digits < 64)),
          bool isFloat = std::is_floating_point<T>::value>
struct ColorTraitsImplementation final
{
    static_assert(isUnsignedInteger || isFloat, "invalid type T");
    static constexpr T max = isFloat ? 1 : std::numeric_limits<T>::max();
    static constexpr std::size_t digits = std::numeric_limits<T>::digits;
    typedef typename std::
        conditional<isFloat,
                    T,
                    typename std::conditional<digits <= 8,
                                              std::uint16_t,
                                              typename std::conditional<digits <= 16,
                                                                        std::uint32_t,
                                                                        std::uint64_t>::type>::
                        type>::type WideType;
};

template <typename T>
struct ColorTraits final
{
    static constexpr T min = 0;
    static constexpr T max = ColorTraitsImplementation<T>::max;

private:
    template <typename T2,
              typename = typename std::enable_if<!std::is_floating_point<T>::value
                                                 && !std::is_floating_point<T2>::value>::type>
    static constexpr T2 castToImplementation(T v, T2) noexcept
    {
        typedef ColorTraits<T2> TargetTraits;
        typedef
            typename std::common_type<WideType, typename TargetTraits::WideType>::type CommonType;
        return static_cast<T2>(static_cast<CommonType>(v) * TargetTraits::max / max);
    }
    template <typename T2,
              typename = typename std::enable_if<std::is_floating_point<T2>::value>::type>
    static constexpr T2 castToImplementation(T v, T2, int = 0) noexcept
    {
        typedef typename std::common_type<T, T2>::type CommonType;
        typedef ColorTraits<T2> TargetTraits;
        return static_cast<T2>(
            static_cast<CommonType>(v)
            * static_cast<CommonType>(TargetTraits::max / static_cast<long double>(max)));
    }
    template <typename T2,
              typename = typename std::enable_if<std::is_floating_point<T>::value
                                                 && !std::is_floating_point<T2>::value>::type>
    static constexpr T2 castToImplementation(T v, T2, long = 0) noexcept
    {
        typedef typename std::common_type<T, T2>::type CommonType;
        typedef ColorTraits<T2> TargetTraits;
        return static_cast<T2>(
            0.5f
            + static_cast<CommonType>(v)
                  * static_cast<CommonType>(TargetTraits::max / static_cast<long double>(max)));
    }
    static constexpr T castToImplementation(T v, T) noexcept
    {
        return v;
    }

public:
    template <typename T2>
    static constexpr T2 castTo(T v) noexcept
    {
        return castToImplementation(v, T2());
    }
    static constexpr T limit(T v) noexcept
    {
        return (constexprAssert(v >= min && v <= max), v);
    }
    static constexpr T multiply(T a, T b) noexcept
    {
        return static_cast<WideType>(a) * b;
    }
    typedef typename ColorTraitsImplementation<T>::WideType WideType;
};

template <typename T>
struct BasicColor final
{
    typedef T ValueType;
    T red;
    T green;
    T blue;
    T opacity;
    constexpr BasicColor() : red(0), green(0), blue(0), opacity(0)
    {
    }
    constexpr explicit BasicColor(T value, T opacity = ColorTraits<T>::max)
        : red(ColorTraits<T>::limit(value)),
          green(ColorTraits<T>::limit(value)),
          blue(ColorTraits<T>::limit(value)),
          opacity(ColorTraits<T>::limit(opacity))
    {
    }
    template <typename T2>
    constexpr explicit BasicColor(BasicColor<T2> v)
        : red(ColorTraits<T2>::template castTo<T>(v.red)),
          green(ColorTraits<T2>::template castTo<T>(v.green)),
          blue(ColorTraits<T2>::template castTo<T>(v.blue)),
          opacity(ColorTraits<T2>::template castTo<T>(v.opacity))
    {
    }
    constexpr BasicColor(BasicColor value, T opacity)
        : red(value.red),
          green(value.green),
          blue(value.blue),
          opacity(ColorTraits<T>::multiply(value.opacity, ColorTraits<T>::limit(opacity)))
    {
    }
    constexpr BasicColor(T red, T green, T blue, T opacity = ColorTraits<T>::max)
        : red(ColorTraits<T>::limit(red)),
          green(ColorTraits<T>::limit(green)),
          blue(ColorTraits<T>::limit(blue)),
          opacity(ColorTraits<T>::limit(opacity))
    {
    }
    friend constexpr BasicColor operator*(BasicColor a, BasicColor b)
    {
        return BasicColor(ColorTraits<T>::multiply(a.red, b.red),
                          ColorTraits<T>::multiply(a.green, b.green),
                          ColorTraits<T>::multiply(a.blue, b.blue),
                          ColorTraits<T>::multiply(a.opacity, b.opacity));
    }
    BasicColor &operator*=(BasicColor rt)
    {
        *this = *this *rt;
        return *this;
    }
    constexpr bool operator==(BasicColor rt) const
    {
        return red == rt.red && green == rt.green && blue == rt.blue && opacity == rt.opacity;
    }
    constexpr bool operator!=(BasicColor rt) const
    {
        return !operator==(rt);
    }
};

template <typename T>
constexpr BasicColor<T> colorize(BasicColor<T> a, BasicColor<T> b)
{
    return a * b;
}

typedef BasicColor<std::uint8_t> ColorI;
typedef BasicColor<float> ColorF;

constexpr ColorI colorize(ColorF a, ColorI b)
{
    return ColorI(colorize(a, ColorF(b)));
}

template <typename T>
constexpr BasicColor<T> grayscaleA(
    typename std::enable_if<true, T>::type value,
    typename std::enable_if<true, T>::type opacity) // use enable_if to prevent type deduction
{
    return BasicColor<T>(value, opacity);
}

constexpr ColorI grayscaleAI(std::uint8_t value, std::uint8_t opacity)
{
    return grayscaleA<std::uint8_t>(value, opacity);
}

constexpr ColorF grayscaleAF(float value, float opacity)
{
    return grayscaleA<float>(value, opacity);
}

template <typename T>
constexpr BasicColor<T> grayscale(
    typename std::enable_if<true, T>::type value) // use enable_if to prevent type deduction
{
    return BasicColor<T>(value);
}

constexpr ColorI grayscaleI(std::uint8_t value)
{
    return grayscale<std::uint8_t>(value);
}

constexpr ColorF grayscaleF(float value)
{
    return grayscale<float>(value);
}

template <typename T>
constexpr BasicColor<T> rgb(
    typename std::enable_if<true, T>::type red,
    typename std::enable_if<true, T>::type green,
    typename std::enable_if<true, T>::type blue) // use enable_if to prevent type deduction
{
    return BasicColor<T>(red, green, blue);
}

constexpr ColorI rgbI(std::uint8_t red, std::uint8_t green, std::uint8_t blue)
{
    return rgb<std::uint8_t>(red, green, blue);
}

constexpr ColorF rgbF(float red, float green, float blue)
{
    return rgb<float>(red, green, blue);
}

template <typename T>
constexpr BasicColor<T> rgba(
    typename std::enable_if<true, T>::type red,
    typename std::enable_if<true, T>::type green,
    typename std::enable_if<true, T>::type blue,
    typename std::enable_if<true, T>::type opacity) // use enable_if to prevent type deduction
{
    return BasicColor<T>(red, green, blue, opacity);
}

constexpr ColorI rgbaI(std::uint8_t red,
                       std::uint8_t green,
                       std::uint8_t blue,
                       std::uint8_t opacity)
{
    return rgba<std::uint8_t>(red, green, blue, opacity);
}

constexpr ColorF rgbaF(float red, float green, float blue, float opacity)
{
    return rgba<float>(red, green, blue, opacity);
}

template <typename T>
constexpr BasicColor<T> colorizeIdentity()
{
    return BasicColor<T>(
        ColorTraits<T>::max, ColorTraits<T>::max, ColorTraits<T>::max, ColorTraits<T>::max);
}

constexpr ColorI colorizeIdentityI()
{
    return colorizeIdentity<std::uint8_t>();
}

constexpr ColorF colorizeIdentityF()
{
    return colorizeIdentity<float>();
}

constexpr ColorF interpolate(float v, ColorF a, ColorF b)
{
    using util::interpolate;
    return ColorF(interpolate(v, a.red, b.red),
                  interpolate(v, a.green, b.green),
                  interpolate(v, a.blue, b.blue),
                  interpolate(v, a.opacity, b.opacity));
}

constexpr ColorI interpolate(float v, ColorI a, ColorI b)
{
    return ColorI(interpolate(v, ColorF(a), ColorF(b)));
}
}
}
}

#endif /* GRAPHICS_COLOR_H_ */
