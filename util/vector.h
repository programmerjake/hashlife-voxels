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
#include <tuple>

namespace programmerjake
{
namespace voxels
{
namespace util
{
template <typename T, std::size_t N>
struct Vector final
{
    static_assert(N >= 1, "");

private:
    T values[N];

private:
    typedef MakeIndexSequence<N> IndexSequenceType;
    template <typename... Args>
    static constexpr typename std::enable_if<sizeof...(Args) + 1 < N, Vector>::type fillHelper(
        T v, Args... args)
    {
        return fillHelper(v, args..., v);
    }
    template <typename... Args>
    static constexpr typename std::enable_if<sizeof...(Args) == N, Vector>::type fillHelper(Args... args)
    {
        return Vector(args...);
    }
    static constexpr bool areAllT()
    {
        return true;
    }
    template <typename Arg, typename... Args>
    static constexpr bool areAllT(Arg, Args... args)
    {
        return std::is_same<Arg, T>::value && areAllT(args...);
    }
    template <std::size_t ...Indices>
    constexpr Vector negHelper(IntegerSequence<std::size_t, Indices...>) const
    {
        return Vector(-values[Indices]...);
    }
    template <std::size_t ...Indices>
    constexpr Vector bitwiseNotHelper(IntegerSequence<std::size_t, Indices...>) const
    {
        return Vector(~values[Indices]...);
    }
    template <std::size_t ...Indices>
    constexpr Vector<bool, N> logicalNotHelper(IntegerSequence<std::size_t, Indices...>) const
    {
        return Vector<bool, N>(!values[Indices]...);
    }

public:
    constexpr explicit Vector(T v = T()) : Vector(fillHelper(v))
    {
    }
    template <typename... Args,
              typename = typename std::enable_if<sizeof...(Args) == N && areAllT(Args()...)>::type>
    constexpr Vector(Args... args)
        : values{args...}
    {
    }
    constexpr const Vector &operator+() const
    {
        return *this;
    }
    constexpr Vector operator-() const
    {
        return negHelper(IndexSequenceType());
    }
    constexpr Vector operator~() const
    {
        return bitwiseNotHelper(IndexSequenceType());
    }
    constexpr Vector<bool, N> operator!() const
    {
        return logicalNotHelper(IndexSequenceType());
    }
    constexpr std::size_t size() const
    {
        return N;
    }
    typedef const T *const_iterator;
    typedef T *iterator;
    constexpr const_iterator begin() const
    {
        return values;
    }
    iterator begin()
    {
        return values;
    }
    constexpr const_iterator cbegin() const
    {
        return values;
    }
    constexpr const_iterator end() const
    {
        return values + N;
    }
    iterator end()
    {
        return values + N;
    }
    constexpr const_iterator cend() const
    {
        return values + N;
    }
    constexpr const T &operator[](std::size_t index) const
    {
        return (constexprAssert(index < N), values[index]);
    }
    T &operator[](std::size_t index)
    {
        return (constexprAssert(index < N), values[index]);
    }
#error finish
};
}
}
}

#endif /* UTIL_VECTOR_H_ */
