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

#ifndef GRAPHICS_DRIVERS_SDL2_DRIVER_H_
#define GRAPHICS_DRIVERS_SDL2_DRIVER_H_

#include "../driver.h"
#include "../../ui/event.h"
#include <SDL.h>

#if SDL_MAJOR_VERSION != 2
#error incorrect SDL version
#endif

namespace programmerjake
{
namespace voxels
{
namespace graphics
{
namespace drivers
{
class SDL2Driver : public Driver
{
private:
    struct RunningState;

private:
    std::string title;
    std::shared_ptr<RunningState> runningState;

protected:
    virtual void renderFrame(std::shared_ptr<CommandBuffer> commandBuffer) = 0;
    virtual SDL_Window *createWindow() = 0; // returns flags
    SDL_Window *getWindow() const noexcept;
    void runOnMainThread(void (*fn)(void *arg), void *arg);
    template <typename Fn>
    void runOnMainThread(Fn &&fn)
    {
        runOnMainThread(
            [](void *arg)
            {
                std::forward<Fn>(*static_cast<typename std::remove_reference<Fn>::type *>(arg))();
            },
            const_cast<void *>(static_cast<const volatile void *>(std::addressof(fn))));
    }

public:
    virtual void run(std::shared_ptr<CommandBuffer>(*renderCallback)(void *arg),
                     void *renderCallbackArg,
                     bool (*eventCallback)(void *arg, const ui::event::Event &event),
                     void *eventCallbackArg) override final;
    const std::string &getTitle() const noexcept
    {
        return title;
    }
    void setTitle(std::string newTitle);
    explicit SDL2Driver(std::string title) : title(std::move(title)), runningState()
    {
    }
    SDL2Driver() : SDL2Driver("Voxels")
    {
    }
};
}
}
}
}

#endif /* GRAPHICS_DRIVERS_SDL2_DRIVER_H_ */
