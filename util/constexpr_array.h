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

#ifndef UTIL_CONSTEXPR_ARRAY_H_
#define UTIL_CONSTEXPR_ARRAY_H_

#include "constexpr_assert.h"
#include <cstdint>
#include <type_traits>
#include <utility>
#include <iterator>
#include <stdexcept>

namespace programmerjake
{
namespace voxels
{
namespace util
{
template <typename T, std::size_t N>
struct Array;

template <typename T>
class ConstArrayIterator;

template <typename T>
class ArrayIterator
{
    template <typename, std::size_t>
    friend struct Array;
    template <typename>
    friend class ConstArrayIterator;

public:
    typedef std::ptrdiff_t difference_type;
    typedef T value_type;
    typedef T *pointer;
    typedef T &reference;
    typedef std::random_access_iterator_tag iterator_category;

private:
    T *value;
    constexpr explicit ArrayIterator(T *value) noexcept : value(value)
    {
    }

public:
    constexpr ArrayIterator() noexcept : value(nullptr)
    {
    }
    ArrayIterator &operator++() noexcept
    {
        value++;
        return *this;
    }
    ArrayIterator &operator--() noexcept
    {
        value--;
        return *this;
    }
    ArrayIterator operator++(int) noexcept
    {
        return iterator(value++);
    }
    ArrayIterator operator--(int) noexcept
    {
        return iterator(value--);
    }
    constexpr T &operator*() const noexcept
    {
        return *value;
    }
    constexpr T *operator->() const noexcept
    {
        return value;
    }
    constexpr T &operator[](std::ptrdiff_t index) const noexcept
    {
        return value[index];
    }
    constexpr bool operator==(const ArrayIterator &rt) const noexcept
    {
        return value == rt.value;
    }
    constexpr bool operator!=(const ArrayIterator &rt) const noexcept
    {
        return value != rt.value;
    }
    constexpr bool operator<(const ArrayIterator &rt) const noexcept
    {
        return value < rt.value;
    }
    constexpr bool operator>(const ArrayIterator &rt) const noexcept
    {
        return value > rt.value;
    }
    constexpr bool operator<=(const ArrayIterator &rt) const noexcept
    {
        return value <= rt.value;
    }
    constexpr bool operator>=(const ArrayIterator &rt) const noexcept
    {
        return value >= rt.value;
    }
    ArrayIterator &operator+=(std::ptrdiff_t offset) noexcept
    {
        value += offset;
        return *this;
    }
    ArrayIterator &operator-=(std::ptrdiff_t offset) noexcept
    {
        value -= offset;
        return *this;
    }
    friend constexpr ArrayIterator operator+(std::ptrdiff_t offset,
                                             const ArrayIterator &iter) noexcept
    {
        return iterator(offset + iter.value);
    }
    friend constexpr ArrayIterator operator+(const ArrayIterator &iter,
                                             std::ptrdiff_t offset) noexcept
    {
        return iterator(iter.value + offset);
    }
    friend constexpr ArrayIterator operator-(const ArrayIterator &iter,
                                             std::ptrdiff_t offset) noexcept
    {
        return iterator(iter.value - offset);
    }
    friend constexpr std::ptrdiff_t operator-(const ArrayIterator &a,
                                              const ArrayIterator &b) noexcept
    {
        return a.value - b.value;
    }
};

template <typename T>
class ConstArrayIterator
{
    template <typename, std::size_t>
    friend struct Array;

public:
    typedef std::ptrdiff_t difference_type;
    typedef T value_type;
    typedef T *pointer;
    typedef T &reference;
    typedef std::random_access_iterator_tag iterator_category;

private:
    const T *value;
    constexpr explicit ConstArrayIterator(const T *value) noexcept : value(value)
    {
    }

public:
    constexpr ConstArrayIterator() noexcept : value(nullptr)
    {
    }
    constexpr ConstArrayIterator(const ArrayIterator<T> &value) noexcept : value(value.value)
    {
    }
    ConstArrayIterator &operator++() noexcept
    {
        value++;
        return *this;
    }
    ConstArrayIterator &operator--() noexcept
    {
        value--;
        return *this;
    }
    ConstArrayIterator operator++(int) noexcept
    {
        return iterator(value++);
    }
    ConstArrayIterator operator--(int) noexcept
    {
        return iterator(value--);
    }
    constexpr const T &operator*() const noexcept
    {
        return *value;
    }
    constexpr const T *operator->() const noexcept
    {
        return value;
    }
    constexpr const T &operator[](std::ptrdiff_t index) const noexcept
    {
        return value[index];
    }
    friend constexpr bool operator==(const ConstArrayIterator &a,
                                     const ConstArrayIterator &b) noexcept
    {
        return a.value == b.value;
    }
    friend constexpr bool operator!=(const ConstArrayIterator &a,
                                     const ConstArrayIterator &b) noexcept
    {
        return a.value != b.value;
    }
    friend constexpr bool operator<(const ConstArrayIterator &a,
                                    const ConstArrayIterator &b) noexcept
    {
        return a.value < b.value;
    }
    friend constexpr bool operator>(const ConstArrayIterator &a,
                                    const ConstArrayIterator &b) noexcept
    {
        return a.value > b.value;
    }
    friend constexpr bool operator<=(const ConstArrayIterator &a,
                                     const ConstArrayIterator &b) noexcept
    {
        return a.value <= b.value;
    }
    friend constexpr bool operator>=(const ConstArrayIterator &a,
                                     const ConstArrayIterator &b) noexcept
    {
        return a.value >= b.value;
    }
    ConstArrayIterator &operator+=(std::ptrdiff_t offset) noexcept
    {
        value += offset;
        return *this;
    }
    ConstArrayIterator &operator-=(std::ptrdiff_t offset) noexcept
    {
        value -= offset;
        return *this;
    }
    friend constexpr ConstArrayIterator operator+(std::ptrdiff_t offset,
                                                  const ConstArrayIterator &iter) noexcept
    {
        return const_iterator(offset + iter.value);
    }
    friend constexpr ConstArrayIterator operator+(const ConstArrayIterator &iter,
                                                  std::ptrdiff_t offset) noexcept
    {
        return const_iterator(iter.value + offset);
    }
    friend constexpr ConstArrayIterator operator-(const ConstArrayIterator &iter,
                                                  std::ptrdiff_t offset) noexcept
    {
        return const_iterator(iter.value - offset);
    }
    friend constexpr std::ptrdiff_t operator-(const ConstArrayIterator &a,
                                              const ConstArrayIterator &b) noexcept
    {
        return a.value - b.value;
    }
};

template <typename T, std::size_t N>
struct ArrayImplementation
{
    typedef T ArrayType[N];
    static constexpr T *getArrayPointer(ArrayType &array) noexcept
    {
        return array;
    }
    static constexpr const T *getArrayPointer(const ArrayType &array) noexcept
    {
        return array;
    }
    static constexpr bool isSwapNoThrow() noexcept
    {
        using std::swap;
        return noexcept(swap(std::declval<T &>(), std::declval<T &>()));
    }
};

template <typename T>
struct ArrayImplementation<T, 0>
{
    struct ArrayType final
    {
    };
    static constexpr T *getArrayPointer(ArrayType &) noexcept
    {
        return static_cast<T *>(nullptr);
    }
    static constexpr const T *getArrayPointer(const ArrayType &) noexcept
    {
        return static_cast<const T *>(nullptr);
    }
    static constexpr bool isSwapNoThrow() noexcept
    {
        return true;
    }
};

template <typename T, std::size_t N>
struct Array
{
    typename ArrayImplementation<T, N>::ArrayType values;
    typedef T value_type;
    typedef std::size_t size_type;
    typedef std::ptrdiff_t difference_type;
    typedef T &reference;
    typedef const T &const_reference;
    typedef T *pointer;
    typedef const T *const_pointer;
    typedef ArrayIterator<T> iterator;
    typedef ConstArrayIterator<T> const_iterator;
    typedef std::reverse_iterator<iterator> reverse_iterator;
    typedef std::reverse_iterator<const_iterator> const_reverse_iterator;
    T &at(std::size_t index)
    {
        return index < N ? ArrayImplementation<T, N>::getArrayPointer(values)[index] :
                           throw std::out_of_range("programmerjake::voxels::util::array.at");
    }
    constexpr const T &at(std::size_t index) const
    {
        return index < N ? ArrayImplementation<T, N>::getArrayPointer(values)[index] :
                           throw std::out_of_range("programmerjake::voxels::util::array.at");
    }
    T &operator[](std::size_t index) noexcept
    {
        return (constexprAssert(index < N),
                ArrayImplementation<T, N>::getArrayPointer(values)[index]);
    }
    constexpr const T &operator[](std::size_t index) const noexcept
    {
        return (constexprAssert(index < N),
                ArrayImplementation<T, N>::getArrayPointer(values)[index]);
    }
    T &front() noexcept
    {
        return operator[](0);
    }
    constexpr const T &front() const noexcept
    {
        return operator[](0);
    }
    T &back() noexcept
    {
        return operator[](N - 1);
    }
    constexpr const T &back() const noexcept
    {
        return operator[](N - 1);
    }
    T *data() noexcept
    {
        return ArrayImplementation<T, N>::getArrayPointer(values);
    }
    constexpr const T *data() const noexcept
    {
        return ArrayImplementation<T, N>::getArrayPointer(values);
    }
    iterator begin() noexcept
    {
        return iterator(data());
    }
    constexpr const_iterator begin() const noexcept
    {
        return const_iterator(data());
    }
    constexpr const_iterator cbegin() const noexcept
    {
        return const_iterator(data());
    }
    iterator end() noexcept
    {
        return iterator(data() + N);
    }
    constexpr const_iterator end() const noexcept
    {
        return const_iterator(data() + N);
    }
    constexpr const_iterator cend() const noexcept
    {
        return const_iterator(data() + N);
    }
    reverse_iterator rbegin() noexcept
    {
        return reverse_iterator(end());
    }
    reverse_iterator rend() noexcept
    {
        return reverse_iterator(begin());
    }
    const_reverse_iterator rbegin() const noexcept
    {
        return const_reverse_iterator(end());
    }
    const_reverse_iterator rend() const noexcept
    {
        return const_reverse_iterator(begin());
    }
    const_reverse_iterator crbegin() const noexcept
    {
        return const_reverse_iterator(end());
    }
    const_reverse_iterator crend() const noexcept
    {
        return const_reverse_iterator(begin());
    }
    constexpr bool empty() const noexcept
    {
        return N == 0;
    }
    constexpr std::size_t size() const noexcept
    {
        return N;
    }
    constexpr std::size_t max_size() const noexcept
    {
        return N;
    }
    void fill(const T &value) noexcept(noexcept(std::declval<T &>() = std::declval<const T &>()))
    {
        for(T &element : *this)
        {
            element = value;
        }
    }
    void swap(Array &other) noexcept(ArrayImplementation<T, N>::isSwapNoThrow())
    {
        for(std::size_t i = 0; i < N; i++)
        {
            using std::swap;
            swap(operator[](i), other[i]);
        }
    }

private:
    constexpr bool equalsHelper(const Array &rt, std::size_t index = 0) const
        noexcept(noexcept(std::declval<const T &>() == std::declval<const T &>() ? 0 : 0))
    {
        return index < N ? (operator[](index) == rt[index] ? equalsHelper(rt, index + 1) : false) :
                           true;
    }
    constexpr bool lessHelper(const Array &rt, std::size_t index = 0) const
        noexcept(noexcept(std::declval<const T &>() < std::declval<const T &>() ? 0 : 0))
    {
        return index < N ?
                   (operator[](index) < rt[index] ?
                        true :
                        (rt[index] < operator[](index) ? false : equalsHelper(rt, index + 1))) :
                   true;
    }

public:
    friend constexpr bool operator==(const Array &a,
                                     const Array &b) noexcept(noexcept(a.equalsHelper(b)))
    {
        return a.equalsHelper(b);
    }
    friend constexpr bool operator!=(const Array &a,
                                     const Array &b) noexcept(noexcept(a.equalsHelper(b)))
    {
        return !a.equalsHelper(b);
    }
    friend constexpr bool operator<(const Array &a,
                                    const Array &b) noexcept(noexcept(a.lessHelper(b)))
    {
        return a.lessHelper(b);
    }
    friend constexpr bool operator>(const Array &a,
                                    const Array &b) noexcept(noexcept(b.lessHelper(a)))
    {
        return b.lessHelper(a);
    }
    friend constexpr bool operator>=(const Array &a,
                                     const Array &b) noexcept(noexcept(a.lessHelper(b)))
    {
        return !a.lessHelper(b);
    }
    friend constexpr bool operator<=(const Array &a,
                                     const Array &b) noexcept(noexcept(b.lessHelper(a)))
    {
        return !b.lessHelper(a);
    }
};
template <typename T, std::size_t N>
void swap(Array<T, N> &a, Array<T, N> &b) noexcept(ArrayImplementation<T, N>::isSwapNoThrow())
{
    a.swap(b);
}
}
}
}

#endif /* UTIL_CONSTEXPR_ARRAY_H_ */
