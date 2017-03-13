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
#include "simd.h"
#include <cstdlib>
#include <iostream>
#include "integer_sequence.h"

namespace programmerjake
{
namespace voxels
{
namespace util
{
#if 1
namespace
{
template <typename T>
void testHelper(T &&)
{
}
template <typename To,
          typename From,
          std::size_t Out = sizeof(To),
          std::size_t In = sizeof(From),
          typename S1 = void,
          typename S2 = void>
struct TestConvert final
{
    static void run() noexcept
    {
        static_assert(In != 0 && Out != 0, "");
        TestConvert<To, From, Out / 2, In / 2>::run();
    }
};
template <typename To, typename From, typename S1, typename S2>
struct TestConvert<To, From, 1, 1, S1, S2> final
{
    static void run() noexcept
    {
        testHelper<SIMD256>(SIMD256::convert<To, From>(SIMD256()));
    }
};
template <std::size_t In, typename To, typename From, typename S1, typename S2>
struct TestConvert<To, From, 1, In, S1, S2> final
{
    static constexpr SIMD256 makeSIMD256(std::size_t) noexcept
    {
        return {};
    }
    template <std::size_t... Values>
    static void run(IntegerSequence<std::size_t, Values...>) noexcept
    {
        testHelper<SIMD256>(SIMD256::convert<To, From>(makeSIMD256(Values)...));
    }
    static void run() noexcept
    {
        run(MakeIndexSequence<In>());
    }
};
template <std::size_t Out, typename To, typename From, typename S1, typename S2>
struct TestConvert<To, From, Out, 1, S1, S2> final
{
    static void run() noexcept
    {
        testHelper<std::array<SIMD256, Out>>(SIMD256::convert<To, From>(SIMD256()));
    }
};
void testSIMD()
{
#define TestConvert1(T)                   \
    TestConvert<T, std::uint8_t>::run();  \
    TestConvert<T, std::int8_t>::run();   \
    TestConvert<T, std::uint16_t>::run(); \
    TestConvert<T, std::int16_t>::run();  \
    TestConvert<T, std::uint32_t>::run(); \
    TestConvert<T, std::int32_t>::run();  \
    TestConvert<T, float>::run();         \
    TestConvert<T, std::uint64_t>::run(); \
    TestConvert<T, std::int64_t>::run();  \
    TestConvert<T, double>::run()
    TestConvert1(std::uint8_t);
    TestConvert1(std::int8_t);
    TestConvert1(std::uint16_t);
    TestConvert1(std::int16_t);
    TestConvert1(std::uint32_t);
    TestConvert1(std::int32_t);
    TestConvert1(float);
    TestConvert1(std::uint64_t);
    TestConvert1(std::int64_t);
    TestConvert1(double);
}
struct TestSIMD final
{
    TestSIMD()
    {
        testSIMD();
        std::exit(0);
    }
} testSIMDInstance;
}
#endif
}
}
}
