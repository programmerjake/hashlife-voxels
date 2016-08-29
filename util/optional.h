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

#ifndef UTIL_OPTIONAL_H_
#define UTIL_OPTIONAL_H_

#include "constexpr_assert.h"
#include <new>
#include <memory>
#include <type_traits>
#include <initializer_list>
#include <utility>
#include <stdexcept>

namespace programmerjake
{
namespace voxels
{
namespace util
{
struct BadOptionalAccess : public std::logic_error
{
    BadOptionalAccess() : logic_error("bad optional access")
    {
    }
};

struct NullOpt
{
    constexpr explicit NullOpt(int)
    {
    }
};

static constexpr NullOpt nullOpt(0);

struct InPlaceT
{
    constexpr explicit InPlaceT(int)
    {
    }
};

static constexpr InPlaceT inPlace(0);

template <typename T>
class Optional;

template <typename T, bool isTriviallyDestructable = std::is_trivially_destructible<T>::value>
class OptionalImplementation final
{
    template <typename>
    friend class Optional;
    OptionalImplementation(const OptionalImplementation &) = delete;
    OptionalImplementation &operator=(const OptionalImplementation &) = delete;

private:
    union
    {
        T fullValue;
        char emptyValue;
    };
    bool isFull;

public:
    constexpr OptionalImplementation() noexcept : emptyValue(), isFull(false)
    {
    }
    template <typename... Args>
    constexpr OptionalImplementation(InPlaceT, Args &&... args) noexcept(
        noexcept(::new(std::declval<void *>()) T(std::declval<Args>()...)))
        : fullValue(std::forward<Args>(args)...), isFull(true)
    {
    }
    ~OptionalImplementation()
    {
        if(isFull)
            fullValue.~T();
    }
    static constexpr bool isSwapNoexcept()
    {
        using std::swap;
        return std::is_nothrow_move_constructible<T>::value
               && noexcept(swap(std::declval<T &>(), std::declval<T &>()));
    }
};

template <typename T>
class OptionalImplementation<T, true> final
{
    template <typename>
    friend class Optional;
    OptionalImplementation(const OptionalImplementation &) = delete;
    OptionalImplementation &operator=(const OptionalImplementation &) = delete;

private:
    union
    {
        T fullValue;
        char emptyValue;
    };
    bool isFull;

public:
    constexpr OptionalImplementation() noexcept : emptyValue(), isFull(false)
    {
    }
    template <typename... Args>
    constexpr explicit OptionalImplementation(InPlaceT, Args &&... args) noexcept(
        noexcept(::new(std::declval<void *>()) T(std::declval<Args>()...)))
        : fullValue(std::forward<Args>(args)...), isFull(true)
    {
    }
    ~OptionalImplementation() = default;
    static constexpr bool isSwapNoexcept()
    {
        using std::swap;
        return std::is_nothrow_move_constructible<T>::value
               && noexcept(swap(std::declval<T &>(), std::declval<T &>()));
    }
};

template <typename T>
class Optional final
{
private:
    OptionalImplementation<T> implementation;

public:
    constexpr Optional() noexcept : implementation()
    {
    }
    constexpr Optional(NullOpt) noexcept : implementation()
    {
    }
    Optional(const Optional &rt) : implementation()
    {
        if(rt.implementation.isFull)
            ::new(static_cast<void *>(std::addressof(implementation.fullValue)))
                T(rt.implementation.fullValue);
        implementation.isFull = rt.implementation.isFull;
    }
    Optional(Optional &&rt) noexcept(std::is_nothrow_move_constructible<T>::value)
        : implementation()
    {
        if(rt.implementation.isFull)
            ::new(static_cast<void *>(std::addressof(implementation.fullValue)))
                T(std::move(rt.implementation.fullValue));
        implementation.isFull = rt.implementation.isFull;
    }
    constexpr Optional(const T &value) : implementation(inPlace, value)
    {
    }
    constexpr Optional(T &&value) : implementation(inPlace, std::move(value))
    {
    }
    template <typename... Args>
    constexpr explicit Optional(InPlaceT, Args &&... args)
        : implementation(inPlace, std::forward<Args>(args)...)
    {
    }
    template <typename U,
              typename... Args,
              typename = typename std::enable_if<std::is_constructible<T,
                                                                       std::initializer_list<U> &,
                                                                       Args &&...>::value>::type>
    constexpr explicit Optional(InPlaceT, std::initializer_list<U> initializerList, Args &&... args)
        : implementation(inPlace, initializerList, std::forward<Args>(args)...)
    {
    }
    Optional &operator=(NullOpt) noexcept
    {
        if(implementation.isFull)
        {
            implementation.isFull = false;
            implementation.fullValue.~T();
        }
        return *this;
    }
    Optional &operator=(const Optional &rt)
    {
        if(!implementation.isFull && rt.implementation.isFull)
        {
            ::new(static_cast<void *>(std::addressof(implementation.fullValue)))
                T(rt.implementation.fullValue);
            implementation.isFull = true;
        }
        else if(implementation.isFull && rt.implementation.isFull)
        {
            implementation.fullValue = rt.implementation.fullValue;
        }
        else
        {
            operator=(nullOpt);
        }
        return *this;
    }
    Optional &operator=(Optional &&rt) noexcept(
        std::is_nothrow_move_assignable<T>::value &&std::is_nothrow_move_constructible<T>::value)
    {
        if(!implementation.isFull && rt.implementation.isFull)
        {
            ::new(static_cast<void *>(std::addressof(implementation.fullValue)))
                T(std::move(rt.implementation.fullValue));
            implementation.isFull = true;
        }
        else if(implementation.isFull && rt.implementation.isFull)
        {
            implementation.fullValue = std::move(rt.implementation.fullValue);
        }
        else
        {
            operator=(nullOpt);
        }
        return *this;
    }
    template <typename U>
    typename std::enable_if<std::is_same<T, typename std::decay<U>::type>::value, Optional &>::type
        operator=(U &&rt)
    {
        if(!implementation.isFull)
        {
            ::new(static_cast<void *>(std::addressof(implementation.fullValue)))
                T(std::forward<U>(rt));
            implementation.isFull = true;
        }
        else
        {
            implementation.fullValue = std::forward<U>(rt);
        }
        return *this;
    }
    constexpr const T *operator->() const
    {
        return (constexprAssert(implementation.isFull), std::addressof(implementation.fullValue));
    }
    T *operator->()
    {
        return (constexprAssert(implementation.isFull), std::addressof(implementation.fullValue));
    }
    constexpr const T &operator*() const &
    {
        return *operator->();
    }
    constexpr const T &&operator*() const &&
    {
        return std::move(*operator->());
    }
    T &operator*() &
    {
        return *operator->();
    }
    T &&operator*() &&
    {
        return std::move(*operator->());
    }
    constexpr explicit operator bool() const noexcept
    {
        return implementation.isFull;
    }
    constexpr bool hasValue() const noexcept
    {
        return implementation.isFull;
    }
    constexpr const T &value() const &
    {
        return *(implementation.isFull ? operator->() : throw BadOptionalAccess());
    }
    constexpr const T &&value() const &&
    {
        return std::move(*(implementation.isFull ? operator->() : throw BadOptionalAccess()));
    }
    T &value() &
    {
        return *(implementation.isFull ? operator->() : throw BadOptionalAccess());
    }
    T &&value() &&
    {
        return std::move(*(implementation.isFull ? operator->() : throw BadOptionalAccess()));
    }
    template <typename U>
    constexpr T valueOr(U &&defaultValue) const &
    {
        return implementation.isFull ? implementation.fullValue :
                                       static_cast<T>(std::forward<U>(defaultValue));
    }
    template <typename U>
    T valueOr(U &&defaultValue) &&
    {
        return implementation.isFull ? std::move(implementation.fullValue) :
                                       static_cast<T>(std::forward<U>(defaultValue));
    }
    template <typename... Args>
    void emplace(Args &&... args)
    {
        operator=(nullOpt);
        ::new(static_cast<void *>(std::addressof(implementation.fullValue)))
            T(std::forward<Args>(args)...);
        implementation.isFull = true;
    }
    template <typename U, typename... Args>
    typename std::enable_if<std::is_constructible<T, std::initializer_list<U> &, Args &&...>::value,
                            void>::type
        emplace(std::initializer_list<U> initializerList, Args &&... args)
    {
        operator=(nullOpt);
        ::new(static_cast<void *>(std::addressof(implementation.fullValue)))
            T(initializerList, std::forward<Args>(args)...);
        implementation.isFull = true;
    }
    void swap(Optional &other) noexcept(OptionalImplementation<T>::isSwapNoexcept())
    {
        if(implementation.isFull)
        {
            if(other.implementation.isFull)
            {
                using std::swap;
                swap(implementation.fullValue, other.implementation.fullValue);
            }
            else
            {
                ::new(static_cast<void *>(std::addressof(other.implementation.fullValue)))
                    T(std::move(implementation.fullValue));
                other.implementation.isFull = true;
                operator=(nullOpt);
            }
        }
        else if(other.implementation.isFull)
        {
            ::new(static_cast<void *>(std::addressof(implementation.fullValue)))
                T(std::move(other.implementation.fullValue));
            implementation.isFull = true;
            other.operator=(nullOpt);
        }
    }
    void reset() noexcept
    {
        operator=(nullOpt);
    }
    friend constexpr bool operator==(const Optional &a, const Optional &b)
    {
        return a.implementation.isFull ?
                   (b.implementation.isFull ?
                        a.implementation.fullValue == b.implementation.fullValue :
                        false) :
                   !b.implementation.isFull;
    }
    friend constexpr bool operator!=(const Optional &a, const Optional &b)
    {
        return a.implementation.isFull ?
                   (b.implementation.isFull ?
                        a.implementation.fullValue != b.implementation.fullValue :
                        true) :
                   b.implementation.isFull;
    }
    friend constexpr bool operator<(const Optional &a, const Optional &b)
    {
        return a.implementation.isFull ?
                   (b.implementation.isFull ?
                        a.implementation.fullValue < b.implementation.fullValue :
                        false) :
                   b.implementation.isFull;
    }
    friend constexpr bool operator>(const Optional &a, const Optional &b)
    {
        return a.implementation.isFull ?
                   (b.implementation.isFull ?
                        a.implementation.fullValue > b.implementation.fullValue :
                        true) :
                   false;
    }
    friend constexpr bool operator<=(const Optional &a, const Optional &b)
    {
        return a.implementation.isFull ?
                   (b.implementation.isFull ?
                        a.implementation.fullValue <= b.implementation.fullValue :
                        false) :
                   true;
    }
    friend constexpr bool operator>=(const Optional &a, const Optional &b)
    {
        return a.implementation.isFull ?
                   (b.implementation.isFull ?
                        a.implementation.fullValue >= b.implementation.fullValue :
                        true) :
                   !b.implementation.isFull;
    }
    friend constexpr bool operator==(NullOpt, const Optional &v) noexcept
    {
        return !v.implementation.isFull;
    }
    friend constexpr bool operator==(const Optional &v, NullOpt) noexcept
    {
        return !v.implementation.isFull;
    }
    friend constexpr bool operator!=(NullOpt, const Optional &v) noexcept
    {
        return v.implementation.isFull;
    }
    friend constexpr bool operator!=(const Optional &v, NullOpt) noexcept
    {
        return v.implementation.isFull;
    }
    friend constexpr bool operator<(NullOpt, const Optional &v) noexcept
    {
        return v.implementation.isFull;
    }
    friend constexpr bool operator<(const Optional &v, NullOpt) noexcept
    {
        return false;
    }
    friend constexpr bool operator>(NullOpt, const Optional &v) noexcept
    {
        return false;
    }
    friend constexpr bool operator>(const Optional &v, NullOpt) noexcept
    {
        return v.implementation.isFull;
    }
    friend constexpr bool operator<=(NullOpt, const Optional &v) noexcept
    {
        return true;
    }
    friend constexpr bool operator<=(const Optional &v, NullOpt) noexcept
    {
        return !v.implementation.isFull;
    }
    friend constexpr bool operator>=(NullOpt, const Optional &v) noexcept
    {
        return !v.implementation.isFull;
    }
    friend constexpr bool operator>=(const Optional &v, NullOpt) noexcept
    {
        return true;
    }
    friend constexpr bool operator==(const T &a, const Optional &b)
    {
        return b.implementation.isFull && a == b.implementation.fullValue;
    }
    friend constexpr bool operator!=(const T &a, const Optional &b)
    {
        return !b.implementation.isFull || a != b.implementation.fullValue;
    }
    friend constexpr bool operator<(const T &a, const Optional &b)
    {
        return b.implementation.isFull && a < b.implementation.fullValue;
    }
    friend constexpr bool operator>(const T &a, const Optional &b)
    {
        return !b.implementation.isFull || a > b.implementation.fullValue;
    }
    friend constexpr bool operator<=(const T &a, const Optional &b)
    {
        return b.implementation.isFull && a <= b.implementation.fullValue;
    }
    friend constexpr bool operator>=(const T &a, const Optional &b)
    {
        return !b.implementation.isFull || a >= b.implementation.fullValue;
    }
    friend constexpr bool operator==(const Optional &a, const T &b)
    {
        return a.implementation.isFull && a.implementation.fullValue == b;
    }
    friend constexpr bool operator!=(const Optional &a, const T &b)
    {
        return !a.implementation.isFull || a.implementation.fullValue != b;
    }
    friend constexpr bool operator<(const Optional &a, const T &b)
    {
        return !a.implementation.isFull || a.implementation.fullValue < b;
    }
    friend constexpr bool operator>(const Optional &a, const T &b)
    {
        return a.implementation.isFull && a.implementation.fullValue > b;
    }
    friend constexpr bool operator<=(const Optional &a, const T &b)
    {
        return !a.implementation.isFull || a.implementation.fullValue <= b;
    }
    friend constexpr bool operator>=(const Optional &a, const T &b)
    {
        return a.implementation.isFull && a.implementation.fullValue >= b;
    }
};

template <typename T>
void swap(Optional<T> &a, Optional<T> &b) noexcept(OptionalImplementation<T>::isSwapNoexcept())
{
    a.swap(b);
}

template <typename T>
constexpr Optional<typename std::decay<T>::type> makeOptional(T &&value)
{
    return Optional<typename std::decay<T>::type>(inPlace, std::forward<T>(value));
}

template <typename T, typename... Args>
constexpr Optional<T> makeOptional(Args &&... args)
{
    return Optional<T>(inPlace, std::forward<Args>(args)...);
}

template <typename T, typename U, typename... Args>
constexpr Optional<T> makeOptional(std::initializer_list<U> initializerList, Args &&... args)
{
    return Optional<T>(inPlace, initializerList, std::forward<Args>(args)...);
}
}
}
}

namespace std
{
template <typename T>
struct hash<programmerjake::voxels::util::Optional<T>>
{
    std::size_t operator()(const programmerjake::voxels::util::Optional<T> &v) const
    {
        if(v.hasValue())
            return std::hash<T>()(v);
        return 0;
    }
};
}

#endif /* UTIL_OPTIONAL_H_ */
