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

#ifndef UTIL_ENUM_H_
#define UTIL_ENUM_H_

#include <type_traits>
#include "constexpr_array.h"
#include "integer_sequence.h"

namespace programmerjake
{
namespace voxels
{
namespace util
{
#define DEFINE_ENUM_MIN(min) programmerjake_voxels_util_enumMinValue = min,
#define DEFINE_ENUM_MAX(max) programmerjake_voxels_util_enumMaxValue = max,
#define DEFINE_ENUM_LIMITS(min, max) \
    DEFINE_ENUM_MIN(min)             \
    DEFINE_ENUM_MAX(max)

template <typename EnumType, bool = std::is_enum<EnumType>::value>
struct EnumTraitsImplementation
{
private:
    struct DetectMin final
    {
        template <typename T, T minValue = T::programmerjake_voxels_util_enumMinValue>
        static std::integral_constant<T, minValue> fn(T *)
        {
            return {};
        }
        static std::integral_constant<EnumType, (0, EnumType())> fn(...)
        {
            return {};
        }
        static constexpr EnumType value = decltype(fn(static_cast<EnumType *>(nullptr)))::value;
    };

public:
    typedef EnumType type;
    static constexpr EnumType min = DetectMin::value;
    static constexpr EnumType max = EnumType::programmerjake_voxels_util_enumMaxValue;
    static_assert(min <= max, "invalid min and max");
    static constexpr std::size_t size =
        static_cast<std::size_t>(max) - static_cast<std::size_t>(min) + 1;
    typedef typename std::underlying_type<EnumType>::type underlying_type;
};

template <typename EnumType>
struct EnumTraitsImplementation<EnumType, false>
{
};

template <typename EnumType>
class EnumIterator;

template <typename EnumType>
struct EnumTraits final
{
    template <typename>
    friend class EnumIterator;

public:
    typedef EnumType type;
    typedef typename EnumTraitsImplementation<EnumType>::underlying_type underlying_type;

private:
    template <underlying_type... values>
    static constexpr Array<EnumType, EnumTraitsImplementation<EnumType>::size> makeValues(
        IntegerSequence<underlying_type, values...>)
    {
        return Array<EnumType, EnumTraitsImplementation<EnumType>::size>{static_cast<EnumType>(
            values + static_cast<underlying_type>(EnumTraitsImplementation<EnumType>::min))...};
    }
    typedef Array<EnumType, EnumTraitsImplementation<EnumType>::size> ValuesImplementationType;
    static constexpr Array<EnumType, EnumTraitsImplementation<EnumType>::size>
        valuesImplementation = makeValues(
            MakeIntegerSequence<underlying_type, EnumTraitsImplementation<EnumType>::size>());

public:
    struct ValuesType final
    {
        typedef EnumIterator<EnumType> iterator;
        typedef EnumIterator<EnumType> const_iterator;
        constexpr const_iterator begin() const;
        constexpr const_iterator end() const;
        constexpr std::size_t size() const
        {
            return EnumTraitsImplementation<EnumType>::size;
        }
    };
    static constexpr ValuesType values{};
    static constexpr std::size_t size = EnumTraitsImplementation<EnumType>::size;
    static constexpr EnumType min = EnumTraitsImplementation<EnumType>::min;
    static constexpr EnumType max = EnumTraitsImplementation<EnumType>::max;
};

template <typename EnumType>
constexpr Array<EnumType, EnumTraitsImplementation<EnumType>::size>
    EnumTraits<EnumType>::valuesImplementation;

template <typename EnumType>
constexpr typename EnumTraits<EnumType>::ValuesType EnumTraits<EnumType>::values;

template <typename EnumType>
class EnumIterator final
{
    template <typename>
    friend struct EnumTraits;

private:
    typedef typename EnumTraits<EnumType>::ValuesImplementationType::const_iterator IteratorType;

public:
    typedef typename std::iterator_traits<IteratorType>::difference_type difference_type;
    typedef typename std::iterator_traits<IteratorType>::value_type value_type;
    typedef typename std::iterator_traits<IteratorType>::pointer pointer;
    typedef typename std::iterator_traits<IteratorType>::reference reference;
    typedef typename std::iterator_traits<IteratorType>::iterator_category iterator_category;
    static_assert(std::is_same<value_type, EnumType>::value, "");
    static_assert(std::is_same<iterator_category, std::random_access_iterator_tag>::value, "");

private:
    IteratorType value;

    constexpr explicit EnumIterator(IteratorType value) : value(value)
    {
    }

public:
    constexpr EnumIterator() : value()
    {
    }
    constexpr explicit EnumIterator(EnumType value)
        : value(EnumTraits<EnumType>::valuesImplementation.begin()
                + (static_cast<std::size_t>(value)
                   - static_cast<std::size_t>(EnumTraits<EnumType>::min)))
    {
    }
    constexpr const EnumType &operator*() const
    {
        return *value;
    }
    constexpr const EnumType &operator[](difference_type v) const
    {
        return value[v];
    }
    constexpr const EnumType *operator->() const
    {
        return &*value;
    }
    EnumIterator &operator++()
    {
        ++value;
        return *this;
    }
    EnumIterator operator++(int)
    {
        return EnumIterator(value++);
    }
    EnumIterator &operator--()
    {
        --value;
        return *this;
    }
    EnumIterator operator--(int)
    {
        return EnumIterator(value--);
    }
    constexpr bool operator==(const EnumIterator &rt) const
    {
        return value == rt.value;
    }
    constexpr bool operator!=(const EnumIterator &rt) const
    {
        return value != rt.value;
    }
    constexpr bool operator<=(const EnumIterator &rt) const
    {
        return value <= rt.value;
    }
    constexpr bool operator>=(const EnumIterator &rt) const
    {
        return value >= rt.value;
    }
    constexpr bool operator<(const EnumIterator &rt) const
    {
        return value < rt.value;
    }
    constexpr bool operator>(const EnumIterator &rt) const
    {
        return value > rt.value;
    }
    EnumIterator &operator+=(difference_type n) const
    {
        value += n;
        return *this;
    }
    EnumIterator &operator-=(difference_type n) const
    {
        value -= n;
        return *this;
    }
    friend constexpr EnumIterator operator+(difference_type n, EnumIterator iter)
    {
        return EnumIterator(n + iter.value + n);
    }
    constexpr EnumIterator operator+(difference_type n) const
    {
        return EnumIterator(value + n);
    }
    constexpr EnumIterator operator-(difference_type n) const
    {
        return EnumIterator(value - n);
    }
    constexpr difference_type operator-(EnumIterator r) const
    {
        return value - r.value;
    }
};

template <typename EnumType>
constexpr typename EnumTraits<EnumType>::ValuesType::const_iterator
    EnumTraits<EnumType>::ValuesType::begin() const
{
    return const_iterator(valuesImplementation.begin());
}

template <typename EnumType>
constexpr typename EnumTraits<EnumType>::ValuesType::const_iterator
    EnumTraits<EnumType>::ValuesType::end() const
{
    return const_iterator(valuesImplementation.end());
}

template <typename T, typename EnumType>
struct EnumArray
{
    typedef EnumType index_type;
    typedef T value_type;
    T values[EnumTraits<EnumType>::size];
    typedef T *iterator;
    typedef const T *const_iterator;
    constexpr std::size_t size() const noexcept
    {
        return EnumTraits<EnumType>::size;
    }
    static_assert(EnumTraits<EnumType>::size > 0, "");
    constexpr bool empty() const noexcept
    {
        return false;
    }
    T *data() noexcept
    {
        return values;
    }
    constexpr const T *data() const noexcept
    {
        return values;
    }
    T &front() noexcept
    {
        return values[0];
    }
    constexpr const T &front() const noexcept
    {
        return values[0];
    }
    T &back() noexcept
    {
        return values[size() - 1];
    }
    constexpr const T &back() const noexcept
    {
        return values[size() - 1];
    }
    iterator begin() noexcept
    {
        return &values[0];
    }
    iterator end() noexcept
    {
        return begin() + size();
    }
    constexpr const_iterator begin() const noexcept
    {
        return &values[0];
    }
    constexpr const_iterator end() const noexcept
    {
        return begin() + size();
    }
    constexpr const_iterator iteratorTo(EnumType index) const noexcept
    {
        return &values[static_cast<typename EnumTraits<EnumType>::underlying_type>(index)
                       - static_cast<typename EnumTraits<EnumType>::underlying_type>(
                             EnumTraits<EnumType>::min)];
    }
    iterator iteratorTo(EnumType index) noexcept
    {
        return &values[static_cast<typename EnumTraits<EnumType>::underlying_type>(index)
                       - static_cast<typename EnumTraits<EnumType>::underlying_type>(
                             EnumTraits<EnumType>::min)];
    }
    constexpr const T &operator[](EnumType index) const noexcept
    {
        return *iteratorTo(index);
    }
    T &operator[](EnumType index) noexcept
    {
        return *iteratorTo(index);
    }
};
}
}
}

#endif /* UTIL_ENUM_H_ */
