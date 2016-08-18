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

#ifndef UTIL_INTEGER_SEQUENCE_H_
#define UTIL_INTEGER_SEQUENCE_H_

#include <cstddef>

namespace programmerjake
{
namespace voxels
{
namespace util
{
template <typename T, T... Ints>
struct IntegerSequence final
{
    typedef T value_type;
    static constexpr std::size_t size()
    {
        return sizeof...(Ints);
    }
};

template <bool isAtEnd, typename T, T N, T... Ints>
struct MakeIntegerSequenceImplementation final
{
    typedef typename MakeIntegerSequenceImplementation<sizeof...(Ints) + 1 >= N,
                                                       T,
                                                       N,
                                                       Ints...,
                                                       sizeof...(Ints)>::type type;
};

template <typename T, T N, T... Ints>
struct MakeIntegerSequenceImplementation<true, T, N, Ints...> final
{
    static_assert(N >= 0, "MakeIntegerSequence: negative sequence length");
    typedef IntegerSequence<T, Ints...> type;
};

template <typename T, T N>
using MakeIntegerSequence = typename MakeIntegerSequenceImplementation<N <= 0, T, N>::type;

template <std::size_t N>
using MakeIndexSequence = MakeIntegerSequence<std::size_t, N>;

template <std::size_t... Ints>
using IndexSequence = IntegerSequence<std::size_t, Ints...>;

template <typename... Args>
using IndexSequenceFor = MakeIndexSequence<sizeof...(Args)>;
}
}
}

#endif /* UTIL_INTEGER_SEQUENCE_H_ */
