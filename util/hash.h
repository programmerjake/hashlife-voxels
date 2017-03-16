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

#ifndef UTIL_HASH_H_
#define UTIL_HASH_H_

#include <limits>
#include <cstdint>
#include <type_traits>
#include "simd.h"

namespace programmerjake
{
namespace voxels
{
namespace util
{
struct FastHashImplementation final
{
    // hashFinalize implementation from MurmurHash3
    // https://github.com/aappleby/smhasher/wiki/MurmurHash3
    template <typename T>
    static typename std::enable_if<(std::numeric_limits<T>::digits > 32), std::size_t>::type
        hashFinalize(T v) noexcept // 64-bit version
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
        hashFinalize(T v, int = 0) noexcept // 32-bit version
    {
        std::uint32_t retval = v;
        retval ^= retval >> 16;
        retval *= 0x85EBCA6BUL;
        retval ^= retval >> 13;
        retval *= 0xC2B2AE35UL;
        retval ^= retval >> 16;
        return retval;
    }
#ifdef __GNUC__
    static constexpr std::uint16_t byteSwap(std::uint16_t v) noexcept
    {
        return __builtin_bswap16(v);
    }
    static constexpr std::uint32_t byteSwap(std::uint32_t v) noexcept
    {
        return __builtin_bswap32(v);
    }
    static constexpr std::uint64_t byteSwap(std::uint64_t v) noexcept
    {
        return __builtin_bswap64(v);
    }
#else
    static constexpr std::uint16_t byteSwap(std::uint16_t v) noexcept
    {
        return (v >> 8) | (v << 8);
    }
    static constexpr std::uint32_t byteSwap(std::uint32_t v) noexcept
    {
        return (static_cast<std::uint32_t>(byteSwap(static_cast<std::uint16_t>(v))) << 16)
               | byteSwap(static_cast<std::uint16_t>(v >> 16));
    }
    static constexpr std::uint64_t byteSwap(std::uint64_t v) noexcept
    {
        return (static_cast<std::uint64_t>(byteSwap(static_cast<std::uint32_t>(v))) << 32)
               | byteSwap(static_cast<std::uint32_t>(v >> 32));
    }
#endif
};

inline std::size_t fastHashFinalize(std::size_t v) noexcept
{
    return FastHashImplementation::hashFinalize(v);
}

struct FastHasher final
{
    std::size_t v;
    constexpr explicit FastHasher(std::size_t v = 0) noexcept : v(v)
    {
    }

private:
    static constexpr std::uint64_t foldWord(std::uint32_t v, std::uint64_t) noexcept
    {
        return v;
    }
    static constexpr std::uint64_t foldWord(std::uint64_t v, std::uint64_t) noexcept
    {
        return v;
    }
    static constexpr std::uint32_t foldWord(std::uint32_t v, std::uint32_t) noexcept
    {
        return v;
    }
    static constexpr std::uint32_t foldWord(std::uint64_t v, std::uint32_t) noexcept
    {
        return v ^ (v >> 32);
    }
    static constexpr std::size_t foldWord(std::uint32_t v) noexcept
    {
        return foldWord(v, static_cast<std::size_t>(0));
    }
    static constexpr std::size_t foldWord(std::uint64_t v) noexcept
    {
        return foldWord(v, static_cast<std::size_t>(0));
    }
    static constexpr int getBitWidth(std::uint64_t) noexcept
    {
        return 64;
    }
    static constexpr int getBitWidth(std::uint32_t) noexcept
    {
        return 32;
    }
    constexpr FastHasher nextHelper(std::size_t value) const noexcept
    {
        return FastHasher((v ^ (v >> getBitWidth(value) / 2))
                              * static_cast<std::size_t>(0x7C942CEE357F35E7ULL)
                          ^ value);
    }

public:
    friend constexpr FastHasher next(FastHasher hasher, std::uint32_t valueIn) noexcept
    {
        return hasher.nextHelper(valueIn);
    }
    friend constexpr FastHasher next(FastHasher hasher, std::uint8_t valueIn) noexcept
    {
        return hasher.nextHelper(valueIn);
    }
    friend constexpr FastHasher next(FastHasher hasher, std::uint16_t valueIn) noexcept
    {
        return hasher.nextHelper(valueIn);
    }
    friend constexpr FastHasher next(FastHasher hasher, std::uint64_t valueIn) noexcept
    {
        return hasher.nextHelper(foldWord(valueIn));
    }
    friend FastHasher next(FastHasher hasher, const volatile void *valueIn) noexcept
    {
        return hasher.nextHelper(foldWord(reinterpret_cast<std::uintptr_t>(valueIn)));
    }
    friend std::size_t finish(FastHasher hasher) noexcept
    {
        return fastHashFinalize(hasher.v);
    }
};
}
}
}

#endif /* UTIL_HASH_H_ */
