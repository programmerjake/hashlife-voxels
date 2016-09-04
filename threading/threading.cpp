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
#include "threading.h"
#include <SDL.h>
#include <atomic>
#include <sstream>
#include <system_error>
#include <limits>
#include <thread>

#if SDL_MAJOR_VERSION != 2
#error incorrect SDL version
#endif

namespace programmerjake
{
namespace voxels
{
namespace threading
{
std::string Thread::makeThreadName()
{
    static std::atomic_uintmax_t nextThreadName(0);
    std::ostringstream ss;
    ss << "Thread" << nextThreadName.fetch_add(1, std::memory_order_relaxed);
    return ss.str();
}

Thread Thread::makeThread(std::unique_ptr<ThreadFunctionBase> fn, const std::string &name)
{
    Thread retval;
    retval.value = static_cast<void *>(SDL_CreateThread(
        [](void *arg) noexcept->int
        {
            std::unique_ptr<ThreadFunctionBase>(static_cast<ThreadFunctionBase *>(arg))->run();
            return 0;
        },
        name.c_str(),
        fn.get()));
    if(!retval.value)
        throw std::system_error(std::errc::resource_unavailable_try_again, SDL_GetError());
    fn.release(); // freed in new thread
    return retval;
}

void Thread::join()
{
    constexprAssert(joinable());
    SDL_WaitThread(static_cast<SDL_Thread *>(value), nullptr);
    value = nullptr;
}

void Thread::detach()
{
    constexprAssert(joinable());
    SDL_DetachThread(static_cast<SDL_Thread *>(value));
    value = nullptr;
}

std::string Thread::getName() const
{
    constexprAssert(joinable());
    auto retval = SDL_GetThreadName(static_cast<SDL_Thread *>(value));
    if(retval)
        return retval;
    return "";
}

static_assert(std::numeric_limits<SDL_threadID>::is_integer, "");
static_assert(!std::numeric_limits<SDL_threadID>::is_signed, "");
static_assert(std::numeric_limits<SDL_threadID>::max()
                  <= std::numeric_limits<std::uintptr_t>::max(),
              "");

Thread::Id Thread::getId() const noexcept
{
    Id retval;
    if(joinable())
    {
        retval.empty = false;
        retval.threadId =
            static_cast<std::uintptr_t>(SDL_GetThreadID(static_cast<SDL_Thread *>(value)));
    }
    return retval;
}

Thread::Id thisThread::getId() noexcept
{
    Thread::Id retval;
    retval.empty = false;
    retval.threadId = static_cast<std::uintptr_t>(SDL_ThreadID());
    return retval;
}
}
}
}
