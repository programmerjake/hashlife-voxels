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
    RunningState *runningState;

protected:
    virtual void renderFrame(std::shared_ptr<CommandBuffer> commandBuffer) = 0;
    virtual SDL_Window *createWindow(int x, int y, int w, int h, std::uint32_t flags) = 0;
    virtual void createGraphicsContext() = 0;
    virtual void destroyGraphicsContext() noexcept = 0;
    SDL_Window *getWindow() const noexcept;
    void runOnMainThread(util::FunctionReference<void()> fn);

public:
    virtual void run(
        util::FunctionReference<std::shared_ptr<CommandBuffer>()> renderCallback,
        util::FunctionReference<void(const ui::event::Event &event)> eventCallback) override final;
    const std::string &getTitle() const noexcept
    {
        return title;
    }
    void setTitle(std::string newTitle);
    explicit SDL2Driver(std::string title) : title(std::move(title)), runningState()
    {
        initSDL();
    }
    SDL2Driver() : SDL2Driver("Voxels")
    {
    }
    static void initSDL() noexcept;
    virtual std::pair<std::size_t, std::size_t> getOutputSize() const noexcept override;
    virtual float getOutputMMPerPixel() const noexcept override;
    virtual void setGraphicsContextRecreationNeeded() noexcept = 0;
    virtual void setRelativeMouseMode(bool enabled) override;
};
}
}
}
}

#endif /* GRAPHICS_DRIVERS_SDL2_DRIVER_H_ */
