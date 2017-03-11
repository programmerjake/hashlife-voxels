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

#ifndef UTIL_ANY_H_
#define UTIL_ANY_H_

#include <type_traits>
#include <typeinfo>
#include <cstddef>
#include <new>
#include <utility>
#include <initializer_list>
#include "constexpr_assert.h"

namespace programmerjake
{
namespace voxels
{
namespace util
{
template <typename T>
struct InPlaceType
{
    explicit InPlaceType() = default;
};

class BadAnyCast : public std::bad_cast
{
};

class Any final
{
private:
    static constexpr std::size_t staticStorageSize = 4 * sizeof(void *);
    static constexpr std::size_t staticStorageAlignment = alignof(std::max_align_t);
    typedef std::aligned_storage<staticStorageSize, staticStorageAlignment>::type StaticStorageType;
    struct ValueBase
    {
        virtual ~ValueBase() = default;
        virtual ValueBase *copy(StaticStorageType &staticStorage) const = 0;
        virtual ValueBase *move(StaticStorageType &staticStorage) noexcept = 0;
        virtual const std::type_info &type() const noexcept = 0;
    };
    template <typename T>
    struct Value final
    {
        static_assert(std::is_move_constructible<T>::value, "T is not move constructible");
        static_assert(std::is_copy_constructible<T>::value, "T is not copy constructible");
        static_assert(std::is_nothrow_destructible<T>::value, "T is not copy constructible");
        static constexpr bool isStatic = sizeof(T) <= staticStorageSize
                                         && alignof(T) <= staticStorageAlignment
                                         && std::is_nothrow_move_constructible<T>::value;
        T value;
        template <typename... Args>
        explicit Value(Args &&... args)
            : value(std::forward<Args>(args)...)
        {
        }
        virtual ValueBase *copy(StaticStorageType &staticStorage) const override
        {
            if(isStatic)
                return ::new(static_cast<void *>(&staticStorage)) Value(*this);
            return new Value(*this);
        }
        virtual ValueBase *move(StaticStorageType &staticStorage) noexcept override
        {
            constexprAssert(isStatic);
            return ::new(static_cast<void *>(&staticStorage)) Value(std::move(*this));
        }
        virtual const std::type_info &type() const noexcept override
        {
            return typeid(T);
        }
    };
    bool isStatic() const noexcept
    {
        return static_cast<void *>(value) == static_cast<void *>(&staticStorage);
    }

private:
    StaticStorageType staticStorage;
    ValueBase *value;

public:
    constexpr Any() noexcept : staticStorage{}, value(nullptr)
    {
    }
    Any(const Any &rt) : Any()
    {
        operator=(rt);
    }
    Any(Any &&rt) noexcept : Any()
    {
        operator=(std::move(rt));
    }
    template <typename T,
              typename = typename std::enable_if<!std::is_same<typename std::decay<T>::type,
                                                               Any>::value>::type>
    Any(T &&v)
        : Any()
    {
        emplace<T>(std::forward<T>(v));
    }
    template <typename T,
              typename = typename std::enable_if<!std::is_same<typename std::decay<T>::type,
                                                               Any>::value>::type>
    Any(T &&v)
        : Any()
    {
        emplace<T>(std::forward<T>(v));
    }
    template <typename T, typename... Args>
    explicit Any(InPlaceType<T>, Args &&... args)
        : Any()
    {
        emplace<T>(std::forward<Args>(args)...);
    }
    template <typename T, typename T2, typename... Args>
    explicit Any(InPlaceType<T>, std::initializer_list<T2> il, Args &&... args)
        : Any()
    {
        emplace<T>(il, std::forward<Args>(args)...);
    }
    ~Any()
    {
        reset();
    }
    Any &operator=(const Any &rt)
    {
        if(this == &rt)
            return *this;
        reset();
        if(rt.value)
            value = rt.value->copy(staticStorage);
        return *this;
    }
    Any &operator=(Any &&rt) noexcept
    {
        reset();
        if(rt.isStatic())
        {
            value = rt.value->move(staticStorage);
            rt.value->~ValueBase();
            rt.value = nullptr;
        }
        else
        {
            value = rt.value;
            rt.value = nullptr;
        }
        return *this;
    }
    void reset() noexcept
    {
        if(value)
        {
            if(isStatic())
                value->~ValueBase();
            else
                delete value;
        }
        value = nullptr;
    }
    void swap(Any &rt) noexcept
    {
        Any temp(std::move(rt));
        rt.operator=(std::move(*this));
        operator=(std::move(temp));
    }
    template <typename T, typename... Args>
    void emplace(Args &&... args)
    {
        reset();
        if(Value<typename std::decay<T>::type>::isStatic)
            value = ::new(static_cast<void *>(&staticStorage))
                Value<typename std::decay<T>::type>(std::forward<Args>(args)...);
        else
            value = new Value<typename std::decay<T>::type>(std::forward<Args>(args)...);
    }
    template <typename T, typename T2, typename... Args>
    void emplace(std::initializer_list<T2> il, Args &&... args)
    {
        reset();
        if(Value<typename std::decay<T>::type>::isStatic)
            value = ::new(static_cast<void *>(&staticStorage))
                Value<typename std::decay<T>::type>(il, std::forward<Args>(args)...);
        else
            value = new Value<typename std::decay<T>::type>(il, std::forward<Args>(args)...);
    }
    bool has_value() const noexcept
    {
        return value != nullptr;
    }
    const std::type_info &type() const noexcept
    {
        if(value)
            return value->type();
        return typeid(void);
    }
    template <typename T>
    friend T *anyCast(Any *v) noexcept;
};

template <typename T>
T *anyCast(Any *v) noexcept
{
    if(!v)
        return nullptr;
    auto *value = dynamic_cast<Any::Value<typename std::decay<T>::type> *>(v->value);
    if(value)
        return value->value;
    return nullptr;
}

template <typename T>
const T *anyCast(const Any *v) noexcept
{
    return anyCast<T>(const_cast<Any *>(v));
}

template <typename T>
T anyCast(const Any &v)
{
    auto *ptr = anyCast<const typename std::remove_reference<T>::type>(&v);
    if(ptr)
        return *ptr;
    throw BadAnyCast();
}

template <typename T>
T anyCast(Any &v)
{
    auto *ptr = anyCast<typename std::remove_reference<T>::type>(&v);
    if(ptr)
        return *ptr;
    throw BadAnyCast();
}

template <typename T>
T anyCast(Any &&v)
{
    auto *ptr = anyCast<typename std::remove_reference<T>::type>(&v);
    if(ptr)
        return *ptr;
    throw BadAnyCast();
}

template <typename T, typename... Args>
Any makeAny(Args &&... args)
{
    return Any(InPlaceType<T>(), std::forward<Args>(args)...);
}

template <typename T, typename T2, typename... Args>
Any makeAny(std::initializer_list<T2> il, Args &&... args)
{
    return Any(InPlaceType<T>(), il, std::forward<Args>(args)...);
}
}
}
}

namespace std
{
inline void swap(programmerjake::voxels::util::Any &a,
                 programmerjake::voxels::util::Any &b) noexcept
{
    a.swap(b);
}
}

#endif /* UTIL_ANY_H_ */
