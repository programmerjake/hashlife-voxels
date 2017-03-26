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
#include "bitset.h"
#include "integer_sequence.h"
#include <cassert>
#include <cstdlib>
#include <iostream>
#include <random>
#include <vector>

namespace programmerjake
{
namespace voxels
{
namespace util
{
#if 1
#warning testing BitSet
struct BitSetNontemplateBase::Tester final
{
    template <std::size_t BitCount>
    static void checkUnusedBits(const BitSet<BitCount> &bitSet)
    {
        constexpr WordType unusedBits =
            BitCount % wordBitCount ? ~((1ULL << (BitCount % wordBitCount)) - 1ULL) : 0;
        assert((bitSet.getWord(bitSet.wordCount - 1) & unusedBits) == 0);
    }
    static void checkUnusedBits(const BitSet<0> &)
    {
    }
    template <std::size_t BitCount>
    static void testDefaultConstruct()
    {
        BitSet<BitCount> bitSet;
        for(std::size_t i = 0; i < bitSet.wordCount; i++)
            assert(bitSet.getWord(i) == 0);
        checkUnusedBits(bitSet);
    }
    template <std::size_t BitCount>
    static void testConstructFromULL()
    {
        for(std::size_t i = 0; i < std::numeric_limits<unsigned long long>::digits; i++)
        {
            BitSet<BitCount> bitSet(1ULL << i);
            checkUnusedBits(bitSet);
            assert(BitSet<BitCount>(1ULL << i).to_ullong() == (1ULL << i) || i >= BitCount);
        }
    }
    template <std::size_t BitCount>
    static void testReferenceAssign()
    {
        std::default_random_engine re;
        std::uniform_int_distribution<unsigned long long> distribution;
        for(std::size_t i = 0; i < 1000; i++)
        {
            BitSet<BitCount> src(distribution(re)), dest;
            for(std::size_t j = 0; j < BitCount; j++)
            {
                dest[j] = src[j];
                checkUnusedBits(src);
                checkUnusedBits(dest);
            }
            assert(src == dest);
        }
    }
    template <std::size_t BitCount>
    static void testReferenceFlip()
    {
        if(BitCount == 0)
            return;
        std::default_random_engine re;
        std::vector<bool> vector;
        vector.resize(BitCount, false);
        BitSet<BitCount> bitSet;
        for(std::size_t i = 0; i < 1000; i++)
        {
            std::size_t index = std::uniform_int_distribution<std::size_t>(0, BitCount - 1)(re);
            vector[index].flip();
            bitSet[index].flip();
            checkUnusedBits(bitSet);
            for(std::size_t j = 0; j < BitCount; j++)
                assert(bitSet[index] == static_cast<bool>(vector[index]));
        }
    }
    template <std::size_t BitCount>
    static void testTest()
    {
        std::default_random_engine re;
        std::vector<bool> vector;
        vector.resize(BitCount, false);
        BitSet<BitCount> bitSet;
        if(BitCount != 0)
        {
            for(std::size_t i = 0; i < 1000; i++)
            {
                std::size_t index = std::uniform_int_distribution<std::size_t>(0, BitCount - 1)(re);
                vector[index].flip();
                bitSet[index].flip();
                checkUnusedBits(bitSet);
            }
        }
        for(std::size_t i = 0; i < BitCount + 1000; i++)
        {
            bool threw = false;
            bool result = false;
            try
            {
                result = bitSet.test(i);
            }
            catch(std::out_of_range &)
            {
                threw = true;
            }
            if(i >= BitCount)
            {
                assert(threw);
            }
            else
            {
                assert(!threw);
                assert(result == vector[i]);
            }
        }
    }
    template <std::size_t BitCount>
    static char test()
    {
        testDefaultConstruct<BitCount>();
        testConstructFromULL<BitCount>();
        testReferenceAssign<BitCount>();
        testReferenceFlip<BitCount>();
        testTest<BitCount>();
        return 0;
    }
    template <typename... Args>
    static void testHelper(Args...)
    {
    }
    template <std::size_t... BitCounts>
    static void test(IntegerSequence<std::size_t, BitCounts...>)
    {
        testHelper(test<BitCounts>()...);
    }
    struct TestRunner final
    {
        TestRunner()
        {
            test(MakeIndexSequence<128>());
            std::exit(0);
        }
    };
    static TestRunner testRunner;
};

BitSetNontemplateBase::Tester::TestRunner BitSetNontemplateBase::Tester::testRunner;
#endif
}
}
}
