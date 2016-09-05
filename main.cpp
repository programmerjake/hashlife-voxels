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
    try
    {
        graphics::MemoryRenderBuffer memoryRenderBuffer;
        block::BlockStepInput blockStepInput;
        for(auto &i : blockStepInput.blocks)
        {
            for(auto &j : i)
            {
                for(auto &b : j)
                {
                    b = block::Block(block::builtin::Air::get()->blockKind,
                                     lighting::Lighting::makeSkyLighting());
                }
            }
        }
        blockStepInput[util::Vector3I32(0)] =
            block::Block(block::builtin::Bedrock::get()->blockKind, lighting::Lighting());
        blockStepInput[util::Vector3I32(0, -1, 0)] =
            block::Block(block::builtin::Air::get()->blockKind,
                         lighting::Lighting(0, lighting::Lighting::maxLight - 1, 0));
        block::builtin::Bedrock::get()->render(
            memoryRenderBuffer,
            blockStepInput,
            block::BlockStepGlobalState(lighting::Lighting::GlobalProperties(
                lighting::Lighting::maxLight, world::Dimension::overworld())));
        auto gpuRenderBuffer =
            graphics::RenderBuffer::makeGPUBuffer(memoryRenderBuffer.getTriangleCounts());
        gpuRenderBuffer->appendBuffer(memoryRenderBuffer, graphics::Transform::translate(-0.5f));
        gpuRenderBuffer->finish();
        graphics::Driver::get().run(
            [&]() -> std::shared_ptr<graphics::Driver::CommandBuffer>
            {
                auto t = std::chrono::duration_cast<std::chrono::duration<double>>(
                             std::chrono::steady_clock::now().time_since_epoch()).count();
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
                commandBuffer->appendRenderCommand(
                    gpuRenderBuffer,
                    graphics::Transform::rotateY(t).concat(graphics::Transform::rotateX(t / 4)),
                    graphics::Transform::translate(0, 0, -2),
                    graphics::Transform::frustum(
                        -0.1 * scaleX, 0.1 * scaleX, -0.1 * scaleY, 0.1 * scaleY, 0.1, 10));
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
