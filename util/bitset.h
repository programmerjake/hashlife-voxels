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

#ifndef UTIL_BITSET_H_
#define UTIL_BITSET_H_

#include <limits>
#include <cstdint>
#include <array>
#include <bitset> // for fast bit count
#include "integer_sequence.h"
#include <initializer_list>
#include "hash.h"

namespace programmerjake
{
namespace voxels
{
namespace util
{
class BitSetNontemplateBaseHelper
{
protected:
    static constexpr bool isPowerOf2(std::size_t v) noexcept
    {
        return (v & (v - 1)) == 0;
    }
};

template <std::size_t BitCount>
class BitSet;

struct BitSetNontemplateBase : public BitSetNontemplateBaseHelper
{
protected:
    struct Tester;

public:
    typedef std::uintptr_t WordType;
    static constexpr std::size_t wordBitCount = std::numeric_limits<WordType>::digits;
    static_assert(isPowerOf2(wordBitCount), "wordBitCount is not a power of 2");
    static constexpr std::size_t constexprMin(std::size_t a, std::size_t b) noexcept
    {
        return a > b ? b : a;
    }
    static constexpr std::size_t getWordIndex(std::size_t bitIndex) noexcept
    {
        return bitIndex / wordBitCount;
    }
    static constexpr WordType getWordMask(std::size_t bitIndex) noexcept
    {
        return static_cast<WordType>(1) << (bitIndex % wordBitCount);
    }
    static constexpr std::size_t getWordCount(std::size_t bitCount) noexcept
    {
        return (bitCount + (wordBitCount - 1)) / wordBitCount;
    }
};

template <std::size_t WordCount>
class BitSetBase : protected BitSetNontemplateBase
{
protected:
    static constexpr std::size_t wordCount = WordCount;
    WordType words[WordCount]; // little endian order
    constexpr BitSetBase() noexcept : words{}
    {
    }
    constexpr BitSetBase(unsigned long long val) noexcept
        : BitSetBase(
              val,
              MakeIndexSequence<constexprMin(
                  getWordCount(std::numeric_limits<unsigned long long>::digits), WordCount)>())
    {
    }
    constexpr WordType getWord(std::size_t wordIndex) const noexcept
    {
        return words[wordIndex];
    }
    void setWord(std::size_t wordIndex, WordType wordValue) noexcept
    {
        words[wordIndex] = wordValue;
    }
    bool equals(const BitSetBase &rt) const noexcept
    {
        for(std::size_t i = 0; i < wordCount; i++)
            if(words[i] != rt.words[i])
                return false;
        return true;
        ;
    }

private:
    template <std::size_t... Indexes>
    constexpr BitSetBase(unsigned long long val, IntegerSequence<std::size_t, Indexes...>) noexcept
        : words{
              static_cast<WordType>(val >> Indexes * wordBitCount)...,
          }
    {
    }
};

template <>
class BitSetBase<0> : protected BitSetNontemplateBase
{
protected:
    static constexpr std::size_t wordCount = 0;
    constexpr BitSetBase() noexcept
    {
    }
    constexpr BitSetBase(unsigned long long) noexcept
    {
    }
    constexpr WordType getWord(std::size_t wordIndex) const noexcept
    {
        return (static_cast<void>(wordIndex), 0);
    }
    void setWord(std::size_t wordIndex, WordType wordValue) noexcept
    {
        static_cast<void>(wordIndex);
        static_cast<void>(wordValue);
    }
    bool equals(const BitSetBase &rt) const noexcept
    {
        return true;
    }

public:
    unsigned long long to_ullong() const
    {
        return 0;
    }
};

template <std::size_t BitCount>
class BitSet final : public BitSetBase<BitSetNontemplateBase::getWordCount(BitCount)>
{
private:
    friend struct BitSetNontemplateBase::Tester;
    static constexpr std::size_t bitCount = BitCount;
    typedef BitSetBase<BitSetNontemplateBase::getWordCount(BitCount)> Base;
    using typename Base::WordType;
    using Base::wordCount;
    using Base::getWord;
    using Base::setWord;
    using Base::getWordCount;
    using Base::getWordMask;
    using Base::getWordIndex;
    using Base::wordBitCount;

private:
    constexpr WordType getWordChecked(std::size_t wordIndex) const noexcept
    {
        return wordIndex < wordCount ? getWord(wordIndex) : 0;
    }

public:
    constexpr BitSet() noexcept : Base()
    {
    }
    constexpr BitSet(unsigned long long val) noexcept
        : Base(bitCount >= std::numeric_limits<unsigned long long>::digits ?
                   val :
                   val & ((1ULL << BitSetNontemplateBase::constexprMin(
                               bitCount, std::numeric_limits<unsigned long long>::digits - 1))
                          - 1ULL))
    {
    }
    class Reference final
    {
        template <std::size_t>
        friend class BitSet;

    private:
        BitSet *bitSet;
        std::size_t bitIndex;
        constexpr Reference(BitSet *bitSet, std::size_t bitIndex) noexcept : bitSet(bitSet),
                                                                             bitIndex(bitIndex)
        {
        }

    public:
        Reference &operator=(
            const Reference &rt) noexcept // assigns referenced value, not this class
        {
            return operator=(static_cast<bool>(rt));
        }
        Reference &operator=(bool value) noexcept
        {
            auto mask = getWordMask(bitIndex);
            auto word = bitSet->getWord(getWordIndex(bitIndex));
            if(value)
                word |= mask;
            else
                word &= ~mask;
            bitSet->setWord(getWordIndex(bitIndex), word);
            return *this;
        }
        constexpr operator bool() const noexcept
        {
            return static_cast<const BitSet *>(bitSet)->operator[](bitIndex);
        }
        constexpr bool operator~() const noexcept
        {
            return !operator bool();
        }
        Reference &flip() noexcept
        {
            auto mask = getWordMask(bitIndex);
            auto word = bitSet->getWord(getWordIndex(bitIndex));
            word ^= mask;
            bitSet->setWord(getWordIndex(bitIndex), word);
            return *this;
        }
    };
    bool operator==(const BitSet &rt) const noexcept
    {
        return this->equals(rt);
    }
    bool operator!=(const BitSet &rt) const noexcept
    {
        return !this->equals(rt);
    }
    Reference operator[](std::size_t bitIndex) noexcept
    {
        return Reference(this, bitIndex);
    }
    constexpr bool operator[](std::size_t bitIndex) const noexcept
    {
        return getWordMask(bitIndex) & getWord(getWordIndex(bitIndex));
    }
    bool test(std::size_t bitIndex) const
    {
        if(bitIndex >= bitCount)
            throw std::out_of_range("bitIndex out of range in BitSet::test");
        return operator[](bitIndex);
    }
    bool all() const noexcept
    {
        if(bitCount == 0)
            return true;
        for(std::size_t i = 0; i < getWordIndex(bitCount - 1); i++)
            if(getWord(i) != static_cast<WordType>(-1))
                return false;
        return getWord(getWordIndex(bitCount - 1))
               == static_cast<WordType>(static_cast<WordType>(getWordMask(bitCount - 1) << 1) - 1);
    }
    bool any() const noexcept
    {
        if(bitCount == 0)
            return false;
        for(std::size_t i = 0; i < getWordIndex(bitCount - 1); i++)
            if(getWord(i) != 0)
                return true;
        return getWord(getWordIndex(bitCount - 1)) != 0;
    }
    bool none() const noexcept
    {
        return !any();
    }
    std::size_t count() const noexcept
    {
        std::size_t retval = 0;
        for(std::size_t i = 0; i < wordCount; i++)
            retval += std::bitset<wordBitCount>(getWord(i)).count();
        return retval;
    }
    constexpr std::size_t size() const noexcept
    {
        return bitCount;
    }
    BitSet &operator&=(const BitSet &rt) noexcept
    {
        for(std::size_t i = 0; i < wordCount; i++)
            setWord(i, getWord(i) & rt.getWord(i));
        return *this;
    }
    friend BitSet operator&(BitSet a, const BitSet &b) noexcept
    {
        return a &= b;
    }
    BitSet &operator|=(const BitSet &rt) noexcept
    {
        for(std::size_t i = 0; i < wordCount; i++)
            setWord(i, getWord(i) | rt.getWord(i));
        return *this;
    }
    friend BitSet operator|(BitSet a, const BitSet &b) noexcept
    {
        return a |= b;
    }
    BitSet &operator^=(const BitSet &rt) noexcept
    {
        for(std::size_t i = 0; i < wordCount; i++)
            setWord(i, getWord(i) ^ rt.getWord(i));
        return *this;
    }
    friend BitSet operator^(BitSet a, const BitSet &b) noexcept
    {
        return a ^= b;
    }
    BitSet &flip() noexcept
    {
        for(std::size_t i = 0; i < getWordIndex(bitCount - 1); i++)
            setWord(i, ~getWord(i));
        setWord(
            getWordIndex(bitCount - 1),
            getWord(getWordIndex(bitCount - 1))
                ^ static_cast<WordType>(static_cast<WordType>(getWordMask(bitCount - 1) << 1) - 1));
        return *this;
    }
    BitSet operator~() const noexcept
    {
        BitSet retval = *this;
        retval.flip();
        return retval;
    }
    BitSet &operator<<=(std::size_t shiftCount) noexcept
    {
        if(shiftCount >= bitCount)
            return reset();
        std::size_t shiftWordCount = shiftCount / wordBitCount;
        std::size_t shiftBitCount = shiftCount % wordBitCount;
        if(shiftBitCount == 0)
        {
            for(std::size_t i = wordCount; i > 0; i--)
            {
                std::size_t index = i - 1;
                setWord(index, getWordChecked(index - shiftWordCount));
            }
        }
        else
        {
            for(std::size_t i = wordCount; i > 0; i--)
            {
                std::size_t index = i - 1;
                WordType highWord = getWordChecked(index - shiftWordCount);
                WordType lowWord = getWordChecked(index - 1 - shiftWordCount);
                setWord(index,
                        (lowWord >> (wordBitCount - shiftBitCount)) | (highWord << shiftBitCount));
            }
        }
        if(wordCount != 0)
            setWord(wordCount - 1,
                    getWord(wordCount - 1)
                        & static_cast<WordType>(
                              static_cast<WordType>(getWordMask(bitCount - 1) << 1) - 1));
        return *this;
    }
    BitSet operator<<(std::size_t shiftCount) const noexcept
    {
        BitSet retval = *this;
        retval <<= shiftCount;
        return retval;
    }
    BitSet &operator>>=(std::size_t shiftCount) noexcept
    {
        if(shiftCount >= bitCount)
            return reset();
        std::size_t shiftWordCount = shiftCount / wordBitCount;
        std::size_t shiftBitCount = shiftCount % wordBitCount;
        if(shiftBitCount == 0)
        {
            for(std::size_t index = 0; index < wordCount; index++)
            {
                setWord(index, getWordChecked(index + shiftWordCount));
            }
        }
        else
        {
            for(std::size_t index = 0; index < wordCount; index++)
            {
                WordType highWord = getWordChecked(index + 1 + shiftWordCount);
                WordType lowWord = getWordChecked(index + shiftWordCount);
                setWord(index,
                        (lowWord >> shiftBitCount) | (highWord << (wordBitCount - shiftBitCount)));
            }
        }
        return *this;
    }
    BitSet operator>>(std::size_t shiftCount) const noexcept
    {
        BitSet retval = *this;
        retval >>= shiftCount;
        return retval;
    }
    BitSet &set() noexcept
    {
        if(wordCount == 0)
            return *this;
        for(std::size_t i = 0; i < getWordIndex(bitCount - 1); i++)
            setWord(i, static_cast<WordType>(-1));
        setWord(getWordIndex(bitCount - 1),
                static_cast<WordType>(static_cast<WordType>(getWordMask(bitCount - 1) << 1) - 1));
        return *this;
    }
    BitSet &set(std::size_t bitIndex, bool value = true)
    {
        if(bitIndex >= bitCount)
            throw std::out_of_range("bitIndex out of range in BitSet::set");
        operator[](bitIndex) = value;
        return *this;
    }
    BitSet &reset() noexcept
    {
        for(std::size_t i = 0; i < wordCount; i++)
            setWord(i, 0);
        return *this;
    }
    BitSet &reset(std::size_t bitIndex)
    {
        if(bitIndex >= bitCount)
            throw std::out_of_range("bitIndex out of range in BitSet::reset");
        operator[](bitIndex) = false;
        return *this;
    }
    BitSet &flip(std::size_t bitIndex)
    {
        if(bitIndex >= bitCount)
            throw std::out_of_range("bitIndex out of range in BitSet::flip");
        operator[](bitIndex).flip();
        return *this;
    }
    unsigned long long to_ullong() const
    {
        unsigned long long retval = 0;
        constexpr std::size_t ullBitCount = std::numeric_limits<unsigned long long>::digits;
        for(std::size_t i = 0; i < wordCount; i++)
        {
            if(i * wordBitCount >= ullBitCount)
            {
                if(getWord(i) != 0)
                    throw std::overflow_error("bit set value too large in BitSet::to_ullong");
            }
            else
            {
                auto word = getWord(i);
                auto shiftedWord = static_cast<unsigned long long>(word) << i * wordBitCount;
                if((shiftedWord >> i * wordBitCount) != word)
                    throw std::overflow_error("bit set value too large in BitSet::to_ullong");
                retval |= shiftedWord;
            }
        }
        return retval;
    }
    unsigned long to_ulong() const
    {
        unsigned long long retval = to_ullong();
        if(retval > std::numeric_limits<unsigned long>::max())
            throw std::overflow_error("bit set value too large in BitSet::to_ulong");
        return retval;
    }
    static constexpr std::size_t npos = -1;
    std::size_t findFirst(bool value, std::size_t start = 0) const noexcept
    {
        if(start >= bitCount)
            return npos;
        constexpr std::size_t endWordIndex = getWordIndex(bitCount - 1);
        std::size_t startWordIndex = getWordIndex(start);
        auto startWord = getWord(startWordIndex);
        if(!value)
        {
            if(startWordIndex == endWordIndex)
                startWord ^= static_cast<WordType>(
                    static_cast<WordType>(getWordMask(bitCount - 1) << 1) - 1);
            else
                startWord = ~startWord;
        }
        auto mask = getWordMask(start);
        for(std::size_t retval = start; mask != 0; mask <<= 1, retval++)
        {
            if(startWord & mask)
                return retval;
        }
        if(startWordIndex == endWordIndex)
            return npos;
        for(std::size_t wordIndex = startWordIndex + 1; wordIndex < endWordIndex; wordIndex++)
        {
            auto word = getWord(wordIndex);
            if(word == static_cast<WordType>(value ? 0 : -1))
                continue;
            if(!value)
                word = ~word;
            mask = 1;
            std::size_t retval = wordIndex * wordBitCount;
            for(; mask != 0; mask <<= 1, retval++)
            {
                if(word & mask)
                    break;
            }
            return retval;
        }
        auto endWord = getWord(endWordIndex);
        if(!value)
            endWord ^=
                static_cast<WordType>(static_cast<WordType>(getWordMask(bitCount - 1) << 1) - 1);
        if(endWord == 0)
            return npos;
        mask = 1;
        std::size_t retval = endWordIndex * wordBitCount;
        for(; mask != 0; mask <<= 1, retval++)
        {
            if(endWord & mask)
                break;
        }
        return retval;
    }
    std::size_t findLast(bool value, std::size_t start = npos) const noexcept
    {
        if(bitCount == 0)
            return npos;
        if(start >= bitCount)
            start = bitCount - 1;
        std::size_t startWordIndex = getWordIndex(start);
        auto startWord = getWord(startWordIndex);
        if(!value)
        {
            if(startWordIndex == getWordIndex(bitCount - 1))
                startWord ^= static_cast<WordType>(
                    static_cast<WordType>(getWordMask(bitCount - 1) << 1) - 1);
            else
                startWord = ~startWord;
        }
        auto mask = getWordMask(start);
        for(std::size_t retval = start; mask != 0; mask >>= 1, retval--)
        {
            if(startWord & mask)
                return retval;
        }
        for(std::size_t wordIndex = startWordIndex - 1, i = 0; i < startWordIndex; wordIndex--, i++)
        {
            auto word = getWord(wordIndex);
            if(word == static_cast<WordType>(value ? 0 : -1))
                continue;
            if(!value)
                word = ~word;
            mask = getWordMask(wordBitCount - 1);
            std::size_t retval = wordIndex * wordBitCount + (wordBitCount - 1);
            for(; mask != 0; mask >>= 1, retval--)
            {
                if(word & mask)
                    break;
            }
            return retval;
        }
        return npos;
    }
    std::size_t hash() const noexcept
    {
        FastHasher hasher;
        for(std::size_t i = 0; i < wordCount; i++)
        {
            hasher = next(hasher, getWord(i));
        }
        return finish(hasher);
    }
};

template <std::size_t BitCount>
constexpr std::size_t BitSet<BitCount>::npos;
}
}
}

namespace std
{
template <std::size_t BitCount>
struct hash<programmerjake::voxels::util::BitSet<BitCount>>
{
    std::size_t operator()(const programmerjake::voxels::util::BitSet<BitCount> &bitSet) const
        noexcept
    {
        return bitSet.hash();
    }
};
}

#endif /* UTIL_BITSET_H_ */
