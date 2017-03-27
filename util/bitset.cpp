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
#include <string>

namespace programmerjake
{
namespace voxels
{
namespace util
{
#if 0
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
    static void testAllNoneAnyAndCountHelper(const std::vector<bool> &vector,
                                             const BitSet<BitCount> &bitSet)
    {
        std::size_t setBitCount = 0;
        for(std::size_t i = 0; i < BitCount; i++)
            if(vector[i])
                setBitCount++;
        assert(bitSet.all() == (setBitCount == BitCount));
        assert(bitSet.any() == (setBitCount != 0));
        assert(bitSet.none() == (setBitCount == 0));
        assert(bitSet.count() == setBitCount);
    }
    template <std::size_t BitCount>
    static void testAllNoneAnyAndCount()
    {
        std::default_random_engine re;
        std::vector<bool> vector;
        vector.resize(BitCount, false);
        BitSet<BitCount> bitSet;
        testAllNoneAnyAndCountHelper(vector, bitSet);
        if(BitCount != 0)
        {
            for(std::size_t i = 0; i < 1000; i++)
            {
                std::size_t index = std::uniform_int_distribution<std::size_t>(0, BitCount - 1)(re);
                vector[index].flip();
                bitSet[index].flip();
                checkUnusedBits(bitSet);
                testAllNoneAnyAndCountHelper(vector, bitSet);
            }
        }
        for(std::size_t i = 0; i < BitCount; i++)
        {
            bitSet[i] = true;
            vector[i] = true;
            checkUnusedBits(bitSet);
            testAllNoneAnyAndCountHelper(vector, bitSet);
        }
    }
    template <std::size_t BitCount>
    static void testAndOrAndXorHelper(const std::vector<bool> &vector1,
                                      const std::vector<bool> &vector2,
                                      const BitSet<BitCount> &bitSet1,
                                      const BitSet<BitCount> &bitSet2)
    {
        BitSet<BitCount> destBitSetAnd = bitSet1 & bitSet2;
        BitSet<BitCount> destBitSetOr = bitSet1 | bitSet2;
        BitSet<BitCount> destBitSetXor = bitSet1 ^ bitSet2;
        checkUnusedBits(destBitSetAnd);
        checkUnusedBits(destBitSetOr);
        checkUnusedBits(destBitSetXor);
        for(std::size_t i = 0; i < BitCount; i++)
        {
            assert(destBitSetAnd[i] == (vector1[i] && vector2[i]));
            assert(destBitSetOr[i] == (vector1[i] || vector2[i]));
            assert(destBitSetXor[i] == (static_cast<bool>(vector1[i]) != vector2[i]));
        }
    }
    template <std::size_t BitCount>
    static void testAndOrAndXor()
    {
        std::default_random_engine re;
        std::vector<bool> vector1, vector2;
        vector1.resize(BitCount, false);
        vector2.resize(BitCount, false);
        BitSet<BitCount> bitSet1, bitSet2;
        testAndOrAndXorHelper(vector1, vector2, bitSet1, bitSet2);
        if(BitCount != 0)
        {
            for(std::size_t i = 0; i < 2000; i++)
            {
                std::size_t index =
                    std::uniform_int_distribution<std::size_t>(0, BitCount * 2 - 1)(re);
                bool isSecondBitSet = index >= BitCount;
                index %= BitCount;
                if(isSecondBitSet)
                {
                    vector2[index].flip();
                    bitSet2[index].flip();
                }
                else
                {
                    vector1[index].flip();
                    bitSet1[index].flip();
                }
                checkUnusedBits(bitSet1);
                checkUnusedBits(bitSet2);
                testAndOrAndXorHelper(vector1, vector2, bitSet1, bitSet2);
            }
        }
        for(std::size_t i = 0; i < BitCount; i++)
        {
            bitSet1[i] = true;
            vector1[i] = true;
            checkUnusedBits(bitSet1);
            checkUnusedBits(bitSet2);
            testAndOrAndXorHelper(vector1, vector2, bitSet1, bitSet2);
        }
        for(std::size_t i = 0; i < BitCount; i++)
        {
            bitSet2[i] = true;
            vector2[i] = true;
            checkUnusedBits(bitSet1);
            checkUnusedBits(bitSet2);
            testAndOrAndXorHelper(vector1, vector2, bitSet1, bitSet2);
        }
    }
    template <std::size_t BitCount>
    static void testNot()
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
                BitSet<BitCount> bitSetNot = ~bitSet;
                checkUnusedBits(bitSetNot);
                for(std::size_t j = 0; j < BitCount; j++)
                    assert(vector[j] == !bitSetNot[j]);
            }
        }
    }
    template <std::size_t BitCount>
    static void testShiftHelper(const std::vector<bool> &vector, const BitSet<BitCount> &bitSet)
    {
        for(std::size_t shiftCount = 0; shiftCount < BitCount * 2 + 1; shiftCount++)
        {
            BitSet<BitCount> bitSetShiftedLeft = bitSet << shiftCount;
            BitSet<BitCount> bitSetShiftedRight = bitSet >> shiftCount;
            checkUnusedBits(bitSetShiftedLeft);
            checkUnusedBits(bitSetShiftedRight);
            for(std::size_t i = 0; i < BitCount; i++)
            {
                assert(bitSetShiftedLeft[i]
                       == (i < shiftCount ? false : static_cast<bool>(vector[i - shiftCount])));
                assert(bitSetShiftedRight[i]
                       == (shiftCount >= BitCount - i ? false :
                                                        static_cast<bool>(vector[i + shiftCount])));
            }
        }
    }
    template <std::size_t BitCount>
    static void testShift()
    {
        std::default_random_engine re;
        std::vector<bool> vector;
        vector.resize(BitCount, false);
        BitSet<BitCount> bitSet;
        testShiftHelper(vector, bitSet);
        if(BitCount != 0)
        {
            for(std::size_t i = 0; i < 1000; i++)
            {
                std::size_t index = std::uniform_int_distribution<std::size_t>(0, BitCount - 1)(re);
                vector[index].flip();
                bitSet[index].flip();
                checkUnusedBits(bitSet);
                testShiftHelper(vector, bitSet);
            }
        }
        for(std::size_t i = 0; i < BitCount; i++)
        {
            bitSet[i] = true;
            vector[i] = true;
            checkUnusedBits(bitSet);
            testShiftHelper(vector, bitSet);
        }
    }
    template <std::size_t BitCount>
    static void testGlobalSetAndReset()
    {
        BitSet<BitCount> bitSet;
        bitSet.reset();
        checkUnusedBits(bitSet);
        assert(bitSet.none());
        bitSet.set();
        checkUnusedBits(bitSet);
        assert(bitSet.all());
    }
    template <std::size_t BitCount>
    static void testFindHelper(const std::string &string, const BitSet<BitCount> &bitSet)
    {
        for(std::size_t i = 0; i < BitCount; i++)
            assert(string[i] == (bitSet[i] ? '1' : '0'));
        for(std::size_t start = 0; start <= BitCount; start++)
        {
            assert(string.find_first_of('1', start) == bitSet.findFirst(true, start));
            assert(string.find_first_not_of('1', start) == bitSet.findFirst(false, start));
            assert(string.find_last_of('1', start) == bitSet.findLast(true, start));
            assert(string.find_last_not_of('1', start) == bitSet.findLast(false, start));
        }
    }
    template <std::size_t BitCount>
    static void testFind()
    {
        std::default_random_engine re;
        std::string string;
        string.resize(BitCount, '0');
        BitSet<BitCount> bitSet;
        testFindHelper(string, bitSet);
        if(BitCount != 0)
        {
            for(std::size_t i = 0; i < 1000; i++)
            {
                std::size_t index = std::uniform_int_distribution<std::size_t>(0, BitCount - 1)(re);
                string[index] = (string[index] == '0' ? '1' : '0');
                bitSet[index].flip();
                checkUnusedBits(bitSet);
                testFindHelper(string, bitSet);
            }
        }
        for(std::size_t i = 0; i < BitCount; i++)
        {
            bitSet[i] = true;
            string[i] = '1';
            checkUnusedBits(bitSet);
            testFindHelper(string, bitSet);
        }
        if(BitCount != 0)
        {
            for(std::size_t i = 0; i < 1000; i++)
            {
                std::size_t index = std::uniform_int_distribution<std::size_t>(0, BitCount - 1)(re);
                string[index] = (string[index] == '0' ? '1' : '0');
                bitSet[index].flip();
                checkUnusedBits(bitSet);
                testFindHelper(string, bitSet);
            }
        }
    }
    template <std::size_t BitCount>
    static char test()
    {
        std::cout << "testing BitSet<" << BitCount << ">" << std::endl;
        testDefaultConstruct<BitCount>();
        testConstructFromULL<BitCount>();
        testReferenceAssign<BitCount>();
        testReferenceFlip<BitCount>();
        testTest<BitCount>();
        testAllNoneAnyAndCount<BitCount>();
        testAndOrAndXor<BitCount>();
        testNot<BitCount>();
        testShift<BitCount>();
        testGlobalSetAndReset<BitCount>();
        testFind<BitCount>();
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
            test(MakeIndexSequence<256>());
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
