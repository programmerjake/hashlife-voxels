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
#include "world/world.h"
#include "world/init.h"
#include "block/block_descriptor.h"
#include "util/enum.h"
#include "logging/logging.h"
#include "util/constexpr_assert.h"
#include "block/builtin/air.h"
#include "graphics/drivers/null_driver.h"
#include "graphics/drivers/opengl_1_driver.h"
#include "block/builtin/bedrock.h"
#include <sstream>
#include <iostream>
#include <chrono>

namespace programmerjake
{
namespace voxels
{
namespace
{
int main()
{
    struct QuitException
    {
    };
    world::initAll(new graphics::drivers::OpenGL1Driver);
    logging::setGlobalLevel(logging::Level::Debug);
    auto theWorld = world::HashlifeWorld::make();
    const std::int32_t renderRange = 32;
    for(std::int32_t x = -renderRange; x < renderRange; x++)
    {
        for(std::int32_t y = -renderRange; y < renderRange; y++)
        {
            for(std::int32_t z = -renderRange; z < renderRange; z++)
            {
                auto block = block::Block(block::builtin::Air::get()->blockKind);
                if(x * x + y * y + z * z < 15 * 15)
                    block = block::Block(block::builtin::Bedrock::get()->blockKind);
                theWorld->setBlock(block, util::Vector3I32(x, y, z));
            }
        }
    }
    block::BlockStepGlobalState blockStepGlobalState(lighting::Lighting::GlobalProperties(
        lighting::Lighting::maxLight, world::Dimension::overworld()));
    try
    {
        std::size_t frameCount = 0;
        auto lastFPSReportTime = std::chrono::steady_clock::now();
        graphics::Driver::get().run(
            [&]() -> std::shared_ptr<graphics::CommandBuffer>
            {
                theWorld->stepAndCollectGarbage(blockStepGlobalState);
                auto now = std::chrono::steady_clock::now();
                frameCount++;
                if(now - lastFPSReportTime >= std::chrono::seconds(5))
                {
                    lastFPSReportTime += std::chrono::seconds(5);
                    std::ostringstream ss;
                    ss << "FPS: " << static_cast<float>(frameCount) / 5;
                    frameCount = 0;
                    logging::log(logging::Level::Info, "main", ss.str());
                }
                auto t = std::chrono::duration_cast<std::chrono::duration<double>>(
                             now.time_since_epoch()).count();
                auto outputSize = graphics::Driver::get().getOutputSize();
                float scaleX = std::get<0>(outputSize);
                float scaleY = std::get<1>(outputSize);
                scaleX /= std::get<1>(outputSize);
                scaleY /= std::get<0>(outputSize);
                if(scaleX < 1)
                    scaleX = 1;
                if(scaleY < 1)
                    scaleY = 1;
                auto commandBuffer = graphics::Driver::get().makeCommandBuffer();
                commandBuffer->appendClearCommand(true, true, graphics::rgbF(0, 0, 0));
                const float nearPlane = 0.01f;
                const float farPlane = 50.0f;
                theWorld->renderView(
                    [&](const std::shared_ptr<world::HashlifeWorld::RenderCacheEntryReference> &
                            renderCacheEntryReference)
                    {
                        return theWorld->renderRenderCacheEntry(renderCacheEntryReference);
                    },
                    util::Vector3F(0.5),
                    farPlane,
                    commandBuffer,
                    graphics::Transform::rotateY(t)
                        .concat(graphics::Transform::rotateX(t / 4))
                        .concat(graphics::Transform::translate(0, 0, 10 * std::sin(t / 3) - 20)),
                    graphics::Transform::frustum(-nearPlane * scaleX,
                                                 nearPlane * scaleX,
                                                 -nearPlane * scaleY,
                                                 nearPlane * scaleY,
                                                 nearPlane,
                                                 farPlane),
                    blockStepGlobalState);
                commandBuffer->appendPresentCommandAndFinish();
                return commandBuffer;
            },
            [](const ui::event::Event &event)
            {
                if(dynamic_cast<const ui::event::Quit *>(&event))
                    throw QuitException();
                if(auto keyDown = dynamic_cast<const ui::event::KeyDown *>(&event))
                {
                    if(keyDown->physicalCode == ui::event::PhysicalKeyCode::Escape)
                        throw QuitException();
                    else if(keyDown->physicalCode == ui::event::PhysicalKeyCode::Space)
                        dynamic_cast<graphics::drivers::SDL2Driver &>(graphics::Driver::get())
                            .setGraphicsContextRecreationNeeded();
                }
            });
    }
    catch(QuitException &)
    {
    }
    return 0;
}
}
}
}

int main()
{
    return programmerjake::voxels::main();
}
