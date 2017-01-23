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

#ifndef THREADING_THREADING_H_
#define THREADING_THREADING_H_

#include "../util/constexpr_assert.h"
#include "../util/integer_sequence.h"
#include <utility>
#include <tuple>
#include <memory>
#include <string>
#include <type_traits>
#include <cstdint>
#include <ostream>
#include <functional>
#include <thread>
#include <chrono>

namespace programmerjake
{
namespace voxels
{
namespace threading
{
namespace thisThread
{
}

class Thread final
{
    Thread(const Thread &) = delete;
    Thread &operator=(const Thread &) = delete;

public:
    class Id;

private:
    void *value;

private:
    struct ThreadFunctionBase
    {
        virtual ~ThreadFunctionBase() = default;
        virtual void run() noexcept = 0;
    };
    template <typename Fn, typename... Args>
    struct ThreadFunction final : public ThreadFunctionBase
    {
        typename std::decay<Fn>::type fn;
        std::tuple<typename std::decay<Args>::type...> args;
        explicit ThreadFunction(Fn &&fn, Args &&... args)
            : fn(std::forward<Fn>(fn)), args(std::forward<Args>(args)...)
        {
        }
        template <std::size_t... indices>
        void run(util::IntegerSequence<std::size_t, indices...>)
        {
            std::move(fn)(std::move(std::get<indices>(args))...);
        }
        virtual void run() noexcept override
        {
            run(util::IndexSequenceFor<Args...>());
        }
    };

private:
    static std::string makeThreadName();
    static Thread makeThread(std::unique_ptr<ThreadFunctionBase> fn, const std::string &name);

public:
    constexpr Thread() noexcept : value(nullptr)
    {
    }
    Thread(Thread &&rt) noexcept : value(rt.value)
    {
        rt.value = nullptr;
    }
    Thread &operator=(Thread &&rt) noexcept
    {
        Thread(std::move(rt)).swap(*this);
        return *this;
    }
    ~Thread()
    {
        constexprAssert(value == nullptr);
    }
    template <typename Fn,
              typename... Args,
              typename = decltype(std::declval<typename std::decay<Fn>::type>()(
                  std::declval<typename std::decay<Args>::type>()...))>
    explicit Thread(const std::string &name, Fn &&fn, Args &&... args)
        : Thread()
    {
        makeThread(std::unique_ptr<ThreadFunctionBase>(new ThreadFunction<Fn, Args...>(
                       std::forward<Fn>(fn), std::forward<Args>(args)...)),
                   name).swap(*this);
    }
    template <typename Fn,
              typename... Args,
              typename = decltype(std::declval<typename std::decay<Fn>::type>()(
                  std::declval<typename std::decay<Args>::type>()...))>
    explicit Thread(Fn &&fn, Args &&... args)
        : Thread(makeThreadName(), std::forward<Fn>(fn), std::forward<Args>(args)...)
    {
    }
    void swap(Thread &rt) noexcept
    {
        std::swap(value, rt.value);
    }
    bool joinable() const noexcept
    {
        return value != nullptr;
    }
    void join();
    void detach();
    std::string getName() const;
    Id getId() const noexcept;
    static std::size_t hardwareConcurrency()
    {
        return std::thread::hardware_concurrency();
    }
};

namespace thisThread
{
Thread::Id getId() noexcept;
inline void yield() noexcept
{
    std::this_thread::yield();
}
template <typename Rep, typename Period>
void sleepFor(const std::chrono::duration<Rep, Period> &sleepDuration)
{
    std::this_thread::sleep_for(sleepDuration);
}
template <typename Clock, typename Duration>
void sleepUntil(const std::chrono::time_point<Clock, Duration> &sleepTime)
{
    std::this_thread::sleep_until(sleepTime);
}
}

class Thread::Id final
{
    friend class Thread;
    friend Id thisThread::getId() noexcept;
    friend struct std::hash<Id>;

private:
    bool empty;
    std::uintptr_t threadId;

public:
    constexpr Id() noexcept : empty(true), threadId()
    {
    }
    friend constexpr bool operator==(Id a, Id b) noexcept
    {
        return a.empty ? b.empty : !b.empty && a.threadId == b.threadId;
    }
    friend constexpr bool operator<(Id a, Id b) noexcept
    {
        return a.empty ? !b.empty : !b.empty && a.threadId < b.threadId;
    }
    friend constexpr bool operator!=(Id a, Id b) noexcept
    {
        return !(a == b);
    }
    friend constexpr bool operator>(Id a, Id b) noexcept
    {
        return b < a;
    }
    friend constexpr bool operator<=(Id a, Id b) noexcept
    {
        return !(a > b);
    }
    friend constexpr bool operator>=(Id a, Id b) noexcept
    {
        return !(a < b);
    }
    template <typename CharT, typename Traits>
    friend std::basic_ostream<CharT, Traits> &operator<<(std::basic_ostream<CharT, Traits> &os,
                                                         Id id)
    {
        if(id.empty)
            return os << "empty";
        return os << id.threadId;
    }
};
}
}
}

namespace std
{
template <>
struct hash<programmerjake::voxels::threading::Thread::Id>
{
    std::size_t operator()(programmerjake::voxels::threading::Thread::Id id) const
    {
        if(id.empty)
            return 0;
        return std::hash<std::uintptr_t>()(id.threadId);
    }
};
}

#endif /* THREADING_THREADING_H_ */
