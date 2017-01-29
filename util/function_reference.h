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

#ifndef UTIL_FUNCTION_REFERENCE_H_
#define UTIL_FUNCTION_REFERENCE_H_

#include <utility>
#include <type_traits>
#include <cstddef>

namespace programmerjake
{
namespace voxels
{
namespace util
{
template <typename Fn>
class FunctionReference
{
public:
    static_assert(std::is_function<Fn>::value, "");
    template <typename Fn2>
    constexpr FunctionReference(Fn2 &&) = delete;
    typedef void ResultType;
    constexpr void operator()() const = delete;
};

template <typename R, typename... Args>
class FunctionReference<R(Args...)> final
{
public:
    typedef R ResultType;

private:
    R (*function)(void *state, Args... args);
    void *state;

public:
    constexpr FunctionReference() noexcept : function(nullptr), state(nullptr)
    {
    }
    constexpr FunctionReference(R (*function)(void *state, Args... args), void *state) noexcept
        : function(function),
          state(state)
    {
    }
    FunctionReference(const FunctionReference &) = default;
    FunctionReference &operator=(const FunctionReference &) = default;
    FunctionReference(FunctionReference &&) = default;
    FunctionReference &operator=(FunctionReference &&) = default;
    template <typename Fn, typename = typename std::enable_if<!std::is_same<typename std::decay<Fn>::type, FunctionReference>::value>::type>
    explicit FunctionReference(Fn &&fn) noexcept
        : function([](void *state, Args... args) -> R
                   {
                       return std::forward<Fn>(*static_cast<typename std::decay<Fn>::type *>(
                                                   state))(std::forward<Args>(args)...);
                   }),
          state(const_cast<void *>(static_cast<const volatile void *>(std::addressof(fn))))
    {
    }
    template <typename R2, typename... Args2>
    explicit FunctionReference(R2 (*function)(Args2...)) noexcept
        : function([](void *state, Args... args) -> R
                   {
                       return reinterpret_cast<R2 (*)(Args2...)>(state)(
                           std::forward<Args>(args)...);
                   }),
          state(reinterpret_cast<void *>(function))
    {
        static_assert(sizeof(void *) == sizeof(R2 (*)(Args2...)), "");
    }
    void swap(FunctionReference &rt) noexcept
    {
        std::swap(function, rt.function);
        std::swap(state, rt.state);
    }
    constexpr explicit operator bool() const noexcept
    {
        return function != nullptr;
    }
    R operator()(Args... args) const
    {
        return function(state, std::forward<Args>(args)...);
    }
    friend constexpr bool operator==(std::nullptr_t,
                                     const FunctionReference<R(Args...)> &v) noexcept
    {
        return !v.operator bool();
    }
    friend constexpr bool operator!=(std::nullptr_t,
                                     const FunctionReference<R(Args...)> &v) noexcept
    {
        return v.operator bool();
    }
    friend constexpr bool operator==(const FunctionReference<R(Args...)> &v,
                                     std::nullptr_t) noexcept
    {
        return !v.operator bool();
    }
    friend constexpr bool operator!=(const FunctionReference<R(Args...)> &v,
                                     std::nullptr_t) noexcept
    {
        return v.operator bool();
    }
};
}
}
}

#endif /* UTIL_FUNCTION_REFERENCE_H_ */
