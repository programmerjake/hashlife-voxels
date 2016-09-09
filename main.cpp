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
#include "util/constexpr_array.h"
#include "block/builtin/air.h"
#include "graphics/drivers/null_driver.h"
#include "graphics/drivers/opengl_1_driver.h"
#include "block/builtin/bedrock.h"
#include "threading/threading.h"
#include <sstream>
#include <iostream>
#include <chrono>
#include <memory>
#include <mutex>
#include <condition_variable>
#include <unordered_map>

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
    constexpr std::int32_t ballSize = 100;
    constexpr std::int32_t renderRange = ballSize + 1;
    typedef util::Array<util::Array<util::Array<block::Block, renderRange * 2>, renderRange * 2>,
                        renderRange * 2> BlocksArray;
    std::unique_ptr<BlocksArray> blocksArray(new BlocksArray);
    for(std::int32_t x = -renderRange; x < renderRange; x++)
    {
        for(std::int32_t y = -renderRange; y < renderRange; y++)
        {
            for(std::int32_t z = -renderRange; z < renderRange; z++)
            {
                auto position = util::Vector3I32(x, y, z);
                auto block = block::Block(block::builtin::Air::get()->blockKind);
                if((position.normSquared() >= ballSize * ballSize
                    && (y < ballSize / 10
                        || util::Vector3F(x, 0, z).normSquared() >= ballSize * ballSize))
                   || (position - util::Vector3I32(ballSize / 2)).normSquared()
                          < ballSize * ballSize / (4 * 4))
                    block = block::Block(block::builtin::Bedrock::get()->blockKind);
                (*blocksArray)[x + renderRange][y + renderRange][z + renderRange] = block;
            }
        }
    }
    theWorld->setBlocks(*blocksArray,
                        util::Vector3I32(-renderRange),
                        util::Vector3I32(0),
                        util::Vector3I32(renderRange * 2));
    blocksArray.reset();
    const block::BlockStepGlobalState blockStepGlobalState(lighting::Lighting::GlobalProperties(
        lighting::Lighting::maxLight, world::Dimension::overworld()));
    struct GenerateRenderBuffersHasher
    {
        std::size_t operator()(
            const std::shared_ptr<world::HashlifeWorld::RenderCacheEntryReference> &v) const
        {
            return v->hash();
        }
    };
    struct GenerateRenderBuffersEquals
    {
        std::size_t operator()(
            const std::shared_ptr<world::HashlifeWorld::RenderCacheEntryReference> &a,
            const std::shared_ptr<world::HashlifeWorld::RenderCacheEntryReference> &b) const
        {
            return *a == *b;
        }
    };
    struct GenerateRenderBuffersValue final
    {
        std::shared_ptr<graphics::ReadableRenderBuffer> renderBuffer = nullptr;
    };
    std::mutex generateRenderBuffersLock;
    std::condition_variable generateRenderBuffersCond;
    std::unordered_map<std::shared_ptr<world::HashlifeWorld::RenderCacheEntryReference>,
                       GenerateRenderBuffersValue,
                       GenerateRenderBuffersHasher,
                       GenerateRenderBuffersEquals> generateRenderBuffersMap;
    std::list<std::shared_ptr<world::HashlifeWorld::RenderCacheEntryReference>>
        generateRenderBuffersWorkList;
    bool generateRenderBuffersDone = false;
    std::list<threading::Thread> generateRenderBuffersThreads;
    const float nearPlane = 0.01f;
    const float farPlane = 5 + ballSize * 2;
    world::HashlifeWorld::GPURenderBufferCache gpuRenderBufferCache;

    std::mutex mainGameLoopLock;
    std::condition_variable mainGameLoopCond;
    bool mainGameLoopDone = false;
    bool mainGameLoopPaused = false;
    threading::Thread mainGameLoopThread;
    try
    {
        std::size_t frameCount = 0;
        auto lastFPSReportTime = std::chrono::steady_clock::now();
        std::size_t tickCount = 0;
        auto lastTPSReportTime = lastFPSReportTime;
        graphics::Driver::get().run(
            [&]() -> std::shared_ptr<graphics::CommandBuffer>
            {
                if(!mainGameLoopThread.joinable())
                {
                    std::size_t totalThreadCount = threading::Thread::hardwareConcurrency();
                    if(totalThreadCount < 3)
                        totalThreadCount = 3;
                    for(std::size_t i = totalThreadCount - 2; i > 0; i--)
                    {
                        generateRenderBuffersThreads.push_back(threading::Thread(
                            [&]()
                            {
                                std::unique_lock<std::mutex> lockIt(generateRenderBuffersLock);
                                while(!generateRenderBuffersDone)
                                {
                                    if(generateRenderBuffersWorkList.empty())
                                    {
                                        generateRenderBuffersCond.wait(lockIt);
                                        continue;
                                    }
                                    auto key = std::move(generateRenderBuffersWorkList.front());
                                    generateRenderBuffersWorkList.pop_front();
                                    lockIt.unlock();
                                    auto result = world::HashlifeWorld::renderRenderCacheEntry(key);
                                    lockIt.lock();
                                    generateRenderBuffersMap[key].renderBuffer = std::move(result);
                                    generateRenderBuffersCond.notify_all();
                                }
                            }));
                    }
                    mainGameLoopThread = threading::Thread(
                        [&]()
                        {
                            std::unique_lock<std::mutex> lockIt(mainGameLoopLock);
                            auto lastTick = std::chrono::high_resolution_clock::now();
                            while(!mainGameLoopDone)
                            {
                                lastTick = std::chrono::high_resolution_clock::now();
                                if(mainGameLoopPaused)
                                {
                                    mainGameLoopCond.wait(lockIt);
                                    continue;
                                }
                                lockIt.unlock();
                                auto now = std::chrono::steady_clock::now();
                                tickCount++;
                                if(now - lastTPSReportTime >= std::chrono::seconds(5))
                                {
                                    lastTPSReportTime += std::chrono::seconds(5);
                                    std::ostringstream ss;
                                    ss << "TPS: " << static_cast<float>(tickCount) / 5;
                                    tickCount = 0;
                                    logging::log(logging::Level::Info, "main", ss.str());
                                }
                                theWorld->stepAndCollectGarbage(blockStepGlobalState);
                                theWorld->updateView(
                                    [&](const std::shared_ptr<world::HashlifeWorld::
                                                                  RenderCacheEntryReference> &key)
                                        -> std::shared_ptr<graphics::ReadableRenderBuffer>
                                    {
                                        std::unique_lock<std::mutex> lockIt(
                                            generateRenderBuffersLock);
                                        auto iter = generateRenderBuffersMap.find(key);
                                        if(iter == generateRenderBuffersMap.end())
                                        {
                                            generateRenderBuffersMap.emplace(
                                                key, GenerateRenderBuffersValue());
                                            generateRenderBuffersWorkList.push_back(key);
                                            generateRenderBuffersCond.notify_all();
                                            return nullptr;
                                        }
                                        if(std::get<1>(*iter).renderBuffer)
                                        {
                                            auto retval =
                                                std::move(std::get<1>(*iter).renderBuffer);
                                            generateRenderBuffersMap.erase(iter);
                                            return retval;
                                        }
                                        return nullptr;
                                    },
                                    [&](const std::shared_ptr<world::HashlifeWorld::
                                                                  GPURenderBufferCache::Entry> &
                                            entry)
                                    {
                                        entry->gpuRenderBuffer =
                                            entry->sourceRenderBuffers.render();
                                    },
                                    util::Vector3F(0.5),
                                    farPlane,
                                    blockStepGlobalState,
                                    gpuRenderBufferCache);
                                // sleep until 1/20 of a second has elapsed
                                // since lastTick
#if 1
                                threading::thisThread::sleepUntil(lastTick
                                                                  + std::chrono::milliseconds(50));
#endif
                                lockIt.lock();
                            }
                        });
                }
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
                gpuRenderBufferCache.renderView(
                    util::Vector3F(0.5),
                    farPlane,
                    commandBuffer,
                    graphics::Transform::rotateY(t / 9).concat(graphics::Transform::rotateX(t / 4)),
                    graphics::Transform::frustum(-nearPlane * scaleX,
                                                 nearPlane * scaleX,
                                                 -nearPlane * scaleY,
                                                 nearPlane * scaleY,
                                                 nearPlane,
                                                 farPlane));
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
    {
        std::unique_lock<std::mutex> lockIt(generateRenderBuffersLock);
        generateRenderBuffersDone = true;
        generateRenderBuffersCond.notify_all();
    }
    {
        std::unique_lock<std::mutex> lockIt(mainGameLoopLock);
        mainGameLoopDone = true;
        mainGameLoopCond.notify_all();
    }
    for(auto &thread : generateRenderBuffersThreads)
    {
        thread.join();
    }
    if(mainGameLoopThread.joinable())
        mainGameLoopThread.join();
    return 0;
}
}
}
}

int main()
{
    return programmerjake::voxels::main();
}
