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

#ifndef UTIL_HASH_H_
#define UTIL_HASH_H_

#include <limits>
#include <cstdint>

namespace programmerjake
{
namespace voxels
{
namespace util
{
struct HashImplementation final
{
    // hashFinalize implementation from MurmurHash3
    // https://github.com/aappleby/smhasher/wiki/MurmurHash3
    template <typename T>
    static typename std::enable_if<(std::numeric_limits<T>::digits > 32), std::size_t>::type
        hashFinalize(T v) // 64-bit version
    {
        std::uint64_t retval = v;
        retval ^= retval >> 33;
        retval *= 0xFF51AFD7ED558CCDULL;
        retval ^= retval >> 33;
        retval *= 0xC4CEB9FE1A85EC53ULL;
        retval ^= retval >> 33;
        return retval;
    }
    template <typename T>
    static typename std::enable_if<(std::numeric_limits<T>::digits <= 32), std::size_t>::type
        hashFinalize(T v, int = 0) // 32-bit version
    {
        std::uint32_t retval = v;
        retval ^= retval >> 16;
        retval *= 0x85EBCA6BUL;
        retval ^= retval >> 13;
        retval *= 0xC2B2AE35UL;
        retval ^= retval >> 16;
        return retval;
    }
};

inline std::size_t hashFinalize(std::size_t v)
{
    return HashImplementation::hashFinalize(v);
}
}
}
}

#endif /* UTIL_HASH_H_ */
