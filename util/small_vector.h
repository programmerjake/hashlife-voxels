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

#ifndef UTIL_SMALL_VECTOR_H_
#define UTIL_SMALL_VECTOR_H_

#include <type_traits>
#include <utility>
#include <new>
#include <iterator>

namespace programmerjake
{
namespace voxels
{
namespace util
{
template <typename T>
class SmallVectorImplementation final
{
    SmallVectorImplementation(const SmallVectorImplementation &) = delete;
    SmallVectorImplementation(SmallVectorImplementation &&) = delete;
    SmallVectorImplementation &operator=(const SmallVectorImplementation &) = delete;
    SmallVectorImplementation &operator=(SmallVectorImplementation &&) = delete;

public:
    typedef typename std::aligned_storage<sizeof(T), alignof(T)>::type storageType;

private:
    storageType *dataArray;
    std::size_t used;
    std::size_t allocated;

public:
    constexpr SmallVectorImplementation(storageType *staticStorage,
                                        std::size_t staticStorageSize) noexcept
        : dataArray(staticStorage),
          used(0),
          allocated(staticStorageSize)
    {
    }
    constexpr static bool isStatic(storageType *staticStorage, storageType *dataArray) const
        noexcept
    {
        return dataArray == staticStorage;
    }
    constexpr bool isStatic(storageType *staticStorage) const noexcept
    {
        return isStatic(staticStorage, dataArray);
    }

private:
    static void reallocateHelper(storageType *oldDataArray,
                                 storageType *newDataArray,
                                 std::size_t used) noexcept
    {
        for(std::size_t i = 0; i < used; i++)
        {
            ::new(static_cast<void *>(newDataArray + i))
                T(std::move(reinterpret_cast<T *>(oldDataArray)[i]));
            reinterpret_cast<T *>(oldDataArray)[i].~T();
        }
    }

public:
    void reallocate(storageType *staticStorage, std::size_t staticStorageSize, std::size_t newSize)
    {
        if(isStatic(staticStorage) && newSize <= staticStorageSize)
            return;
        storageType *oldDataArray = dataArray;
        dataArray = newSize <= staticStorageSize ? staticStorage : new storageType[newSize];
        reallocateHelper(oldDataArray, dataArray, used);
        allocated = newSize;
        if(!isStatic(staticStorage, oldDataArray))
            delete[] oldDataArray;
    }
    const T *data() const noexcept
    {
        return reinterpret_cast<T *>(dataArray);
    }
    T *data() noexcept
    {
        return reinterpret_cast<T *>(dataArray);
    }
    constexpr std::size_t size() const noexcept
    {
        return used;
    }
    constexpr std::size_t capacity() const noexcept
    {
        return allocated;
    }
    template <typename IteratorValueType>
    class Iterator
    {
        template <typename>
        friend class SmallVectorImplementation;
        template <typename>
        friend class Iterator;

    public:
        typedef std::ptrdiff_t difference_type;
        typedef typename std::remove_const<IteratorValueType>::type value_type;
        typedef std::random_access_iterator_tag iterator_category;
        typedef IteratorValueType &reference;
        typedef IteratorValueType *pointer;

    private:
        IteratorValueType *position;

    public:
        constexpr Iterator() noexcept : position(nullptr)
        {
        }
        template <typename = typename std::enable_if<std::is_const<IteratorValueType>::value>::type>
        constexpr Iterator(
            const Iterator<typename std::remove_const<IteratorValueType>::type> &rt) noexcept
            : position(rt.position)
        {
        }

    private:
        constexpr explicit Iterator(IteratorValueType *position) noexcept : position(position)
        {
        }

    public:
        Iterator &operator++() noexcept
        {
            position++;
            return *this;
        }
        Iterator operator++(int) noexcept
        {
            return Iterator(position++);
        }
        Iterator &operator--() noexcept
        {
            position--;
            return *this;
        }
        Iterator operator--(int) noexcept
        {
            return Iterator(position++);
        }
        IteratorValueType &operator*() const noexcept
        {
            return *position;
        }
        IteratorValueType &operator[](std::ptrdiff_t index) const noexcept
        {
            return position[index];
        }
        IteratorValueType *operator->() const noexcept
        {
            return position;
        }
        template <typename T1,
                  typename T2,
                  typename = typename std::enable_if<std::is_same<const T1, const T2>::value>::type>
        friend constexpr bool operator==(const Iterator<T1> &a, const Iterator<T2> &b) noexcept
        {
            return a.position == b.position;
        }
        template <typename T1,
                  typename T2,
                  typename = typename std::enable_if<std::is_same<const T1, const T2>::value>::type>
        friend constexpr bool operator!=(const Iterator<T1> &a, const Iterator<T2> &b) noexcept
        {
            return a.position != b.position;
        }
        template <typename T1,
                  typename T2,
                  typename = typename std::enable_if<std::is_same<const T1, const T2>::value>::type>
        friend constexpr bool operator>=(const Iterator<T1> &a, const Iterator<T2> &b) noexcept
        {
            return a.position >= b.position;
        }
        template <typename T1,
                  typename T2,
                  typename = typename std::enable_if<std::is_same<const T1, const T2>::value>::type>
        friend constexpr bool operator<=(const Iterator<T1> &a, const Iterator<T2> &b) noexcept
        {
            return a.position <= b.position;
        }
        template <typename T1,
                  typename T2,
                  typename = typename std::enable_if<std::is_same<const T1, const T2>::value>::type>
        friend constexpr bool operator>(const Iterator<T1> &a, const Iterator<T2> &b) noexcept
        {
            return a.position > b.position;
        }
        template <typename T1,
                  typename T2,
                  typename = typename std::enable_if<std::is_same<const T1, const T2>::value>::type>
        friend constexpr bool operator<(const Iterator<T1> &a, const Iterator<T2> &b) noexcept
        {
            return a.position < b.position;
        }
        template <typename T1,
                  typename T2,
                  typename = typename std::enable_if<std::is_same<const T1, const T2>::value>::type>
        friend constexpr std::ptrdiff_t operator-(const Iterator<T1> &a,
                                                  const Iterator<T2> &b) noexcept
        {
            return a.position - b.position;
        }
        template <typename T1>
        friend constexpr Iterator<T1> operator+(std::ptrdiff_t a, const Iterator<T1> &b) noexcept
        {
            return Iterator<T1>(a + b.position);
        }
        constexpr Iterator operator+(std::ptrdiff_t b) const noexcept
        {
            return Iterator(position + b);
        }
        constexpr Iterator operator-(std::ptrdiff_t b) const noexcept
        {
            return Iterator(position - b);
        }
        Iterator &operator+=(std::ptrdiff_t b) noexcept
        {
            position += b;
            return *this;
        }
        Iterator &operator-=(std::ptrdiff_t b) noexcept
        {
            position -= b;
            return *this;
        }
    };
    typedef Iterator<T> iterator;
    typedef Iterator<const T> const_iterator;
    iterator begin() noexcept
    {
        return iterator(reinterpret_cast<T *>(dataArray));
    }
    iterator end() noexcept
    {
        return iterator(reinterpret_cast<T *>(dataArray + used));
    }
    const_iterator begin() const noexcept
    {
        return const_iterator(reinterpret_cast<const T *>(dataArray));
    }
    const_iterator end() const noexcept
    {
        return const_iterator(reinterpret_cast<const T *>(dataArray + used));
    }
    const_iterator cbegin() const noexcept
    {
        return const_iterator(reinterpret_cast<const T *>(dataArray));
    }
    const_iterator cend() const noexcept
    {
        return const_iterator(reinterpret_cast<const T *>(dataArray + used));
    }
    typedef std::reverse_iterator<iterator> reverse_iterator;
    typedef std::reverse_iterator<const_iterator> const_reverse_iterator;
#error finish
};

template <typename T, std::size_t StaticValueCount>
class SmallVector final
{
private:
    typename SmallVectorImplementation<T>::storageType staticStorage[StaticValueCount];
#error finish
};
}
}
}

#endif /* UTIL_SMALL_VECTOR_H_ */
