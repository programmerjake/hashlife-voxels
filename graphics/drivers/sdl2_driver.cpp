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
#include "sdl2_driver.h"
#include "../../threading/threading.h"
#include <system_error>
#include <mutex>
#include <condition_variable>
#include <deque>

namespace programmerjake
{
namespace voxels
{
namespace graphics
{
namespace drivers
{
struct SDL2Driver::RunningState final
{
    SDL2Driver &driver;
    std::mutex mutex;
    std::condition_variable cond;
    std::exception_ptr returnedException;
    bool done;
    const threading::Thread::Id mainThreadId = threading::thisThread::getId();
    SDL_Window *window;
    struct ScheduledCallback final
    {
        void (*fn)(void *arg);
        void *arg;
        ScheduledCallback(void (*fn)(void *arg), void *arg) : fn(fn), arg(arg)
        {
        }
    };
    std::deque<ScheduledCallback> scheduledCallbacks;
    explicit RunningState(SDL2Driver &driver)
        : driver(driver), mutex(), cond(), returnedException(), done(false), window()
    {
    }
    void run(std::shared_ptr<CommandBuffer>(*renderCallback)(void *arg),
             void *renderCallbackArg,
             bool (*eventCallback)(void *arg, const ui::event::Event &event),
             void *eventCallbackArg)
    {
        window = driver.createWindow();
#error finish
    }
};

SDL_Window *SDL2Driver::getWindow() const noexcept
{
    constexprAssert(runningState);
    return runningState->window;
}

void SDL2Driver::runOnMainThread(void (*fn)(void *arg), void *arg)
{
    constexprAssert(runningState != nullptr);
    if(threading::thisThread::getId() == runningState->mainThreadId)
    {
        fn(arg);
    }
    else
    {
#error finish
    }
}

void SDL2Driver::run(std::shared_ptr<CommandBuffer>(*renderCallback)(void *arg),
                     void *renderCallbackArg,
                     bool (*eventCallback)(void *arg, const ui::event::Event &event),
                     void *eventCallbackArg)
{
    constexprAssert(runningState == nullptr);
    runningState = std::make_shared<RunningState>(*this);
    try
    {
        runningState->run(renderCallback, renderCallbackArg, eventCallback, eventCallbackArg);
    }
    catch(...)
    {
        runningState = nullptr;
        throw;
    }
    runningState = nullptr;
}
}
}
}
}
