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
    struct DeferredBlocksArray
    {
        block::Block getBlock(util::Vector3I32 position)
        {
            position -= util::Vector3I32(renderRange);
            if((position.normSquared() >= ballSize * ballSize
                && (position.y < (position.x > 0 ? 0 : ballSize * 3 / 4)
                    || util::Vector3F(position.x, 0, position.z).normSquared() >= ballSize
                                                                                      * ballSize))
               || ((position - util::Vector3I32(ballSize / 2)).normSquared() < ballSize * ballSize
                                                                                   / (4 * 4)
                   && (position.x % 32 != 0 || position.z % 32 != 0)))
                return block::Block(block::builtin::Bedrock::get()->blockKind);
            return block::Block(block::builtin::Air::get()->blockKind,
                                lighting::Lighting::makeSkyLighting());
        }
        constexpr std::size_t size() const noexcept
        {
            return renderRange * 2;
        }
        struct IndexHelper2
        {
            DeferredBlocksArray &deferredBlocksArray;
            std::int32_t x, y;
            constexpr IndexHelper2(DeferredBlocksArray &deferredBlocksArray,
                                   std::int32_t x,
                                   std::int32_t y)
                : deferredBlocksArray(deferredBlocksArray), x(x), y(y)
            {
            }
            block::Block operator[](std::int32_t z)
            {
                return deferredBlocksArray.getBlock({x, y, z});
            }
            constexpr std::size_t size() const noexcept
            {
                return renderRange * 2;
            }
        };
        struct IndexHelper1
        {
            DeferredBlocksArray &deferredBlocksArray;
            std::int32_t x;
            constexpr IndexHelper1(DeferredBlocksArray &deferredBlocksArray, std::int32_t x)
                : deferredBlocksArray(deferredBlocksArray), x(x)
            {
            }
            IndexHelper2 operator[](std::int32_t y)
            {
                return IndexHelper2(deferredBlocksArray, x, y);
            }
            constexpr std::size_t size() const noexcept
            {
                return renderRange * 2;
            }
        };
        IndexHelper1 operator[](std::int32_t x)
        {
            return IndexHelper1(*this, x);
        }
    };
    theWorld->setBlocks(DeferredBlocksArray(),
                        util::Vector3I32(-renderRange),
                        util::Vector3I32(0),
                        util::Vector3I32(renderRange * 2));
    const block::BlockStepGlobalState blockStepGlobalState(lighting::Lighting::GlobalProperties(
        lighting::Lighting::maxLight, world::Dimension::overworld()));
#if 0
    for(std::size_t i = 0; i < ballSize; i++)
    {
        theWorld->stepAndCollectGarbage(blockStepGlobalState);
    }
#endif
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
    const float farPlane = 15 + ballSize * 2;
    world::HashlifeWorld::GPURenderBufferCache gpuRenderBufferCache;
    util::Vector3F playerPosition(0.5f);
    float viewPhi = 0;
    float viewTheta = 0;
    float deltaViewPhi = 0;
    float deltaViewTheta = 0;
    bool keyWPressed = false;
    bool keyAPressed = false;
    bool keySPressed = false;
    bool keyDPressed = false;
    bool keySpacePressed = false;
    bool keyLeftShiftPressed = false;
    bool keyRightShiftPressed = false;

    std::mutex mainGameLoopLock;
    std::condition_variable mainGameLoopCond;
    bool mainGameLoopDone = false;
    bool mainGameLoopPaused = false;
    util::Vector3F mainGameLoopPlayerPosition = playerPosition;
    threading::Thread mainGameLoopThread;
    try
    {
        std::size_t frameCount = 0;
        auto lastFPSReportTime = std::chrono::steady_clock::now();
        std::size_t tickCount = 0;
        auto lastTPSReportTime = lastFPSReportTime;
        auto lastFrameTime = lastFPSReportTime;
        graphics::Driver::get().run(
            [&]() -> std::shared_ptr<graphics::CommandBuffer>
            {
                if(!mainGameLoopThread.joinable())
                {
#if 0
                    graphics::Driver::get().setRelativeMouseMode(true);
#endif
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
                                auto playerPosition = mainGameLoopPlayerPosition;
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
                                    playerPosition,
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
                auto deltaTime = std::chrono::duration_cast<std::chrono::duration<double>>(
                                     now - lastFrameTime).count();
                lastFrameTime = now;
                if(deltaTime > 0.1f)
                    deltaTime = 0.1f;
                frameCount++;
                if(now - lastFPSReportTime >= std::chrono::seconds(5))
                {
                    lastFPSReportTime += std::chrono::seconds(5);
                    std::ostringstream ss;
                    ss << "FPS: " << static_cast<float>(frameCount) / 5;
                    frameCount = 0;
                    logging::log(logging::Level::Info, "main", ss.str());
                }
                deltaViewPhi *= 0.5f;
                viewPhi += deltaViewPhi;
                if(viewPhi < -M_PI_2)
                    viewPhi = -M_PI_2;
                else if(!(viewPhi < M_PI_2)) // handle NaN
                    viewPhi = M_PI_2;
                deltaViewTheta *= 0.5f;
                viewTheta += deltaViewTheta;
                util::Vector3F playerRelativeMoveDirection(0);
                if(keyWPressed)
                    playerRelativeMoveDirection -= util::Vector3F(0, 0, 1);
                if(keyAPressed)
                    playerRelativeMoveDirection -= util::Vector3F(1, 0, 0);
                if(keySPressed)
                    playerRelativeMoveDirection += util::Vector3F(0, 0, 1);
                if(keyDPressed)
                    playerRelativeMoveDirection += util::Vector3F(1, 0, 0);
                if(keyLeftShiftPressed || keyRightShiftPressed)
                    playerRelativeMoveDirection -= util::Vector3F(0, 1, 0);
                if(keySpacePressed)
                    playerRelativeMoveDirection += util::Vector3F(0, 1, 0);
                playerPosition += transform(
                    graphics::Transform::rotateY(viewTheta),
                    util::Vector3F(deltaTime * ballSize / 10) * playerRelativeMoveDirection);
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
                commandBuffer->appendClearCommand(true, true, graphics::rgbF(0.5f, 0.5f, 1));
                gpuRenderBufferCache.renderView(playerPosition,
                                                farPlane,
                                                commandBuffer,
                                                graphics::Transform::rotateY(-viewTheta)
                                                    .concat(graphics::Transform::rotateX(-viewPhi)),
                                                graphics::Transform::frustum(-nearPlane * scaleX,
                                                                             nearPlane * scaleX,
                                                                             -nearPlane * scaleY,
                                                                             nearPlane * scaleY,
                                                                             nearPlane,
                                                                             farPlane));
                commandBuffer->appendPresentCommandAndFinish();
                return commandBuffer;
            },
            [&](const ui::event::Event &event)
            {
                if(dynamic_cast<const ui::event::Quit *>(&event))
                    throw QuitException();
                if(auto keyDown = dynamic_cast<const ui::event::KeyDown *>(&event))
                {
                    if(keyDown->physicalCode == ui::event::PhysicalKeyCode::Escape)
                        throw QuitException();
                    else if(keyDown->physicalCode == ui::event::PhysicalKeyCode::R)
                        dynamic_cast<graphics::drivers::SDL2Driver &>(graphics::Driver::get())
                            .setGraphicsContextRecreationNeeded();
                    else if(keyDown->physicalCode == ui::event::PhysicalKeyCode::W)
                        keyWPressed = true;
                    else if(keyDown->physicalCode == ui::event::PhysicalKeyCode::A)
                        keyAPressed = true;
                    else if(keyDown->physicalCode == ui::event::PhysicalKeyCode::S)
                        keySPressed = true;
                    else if(keyDown->physicalCode == ui::event::PhysicalKeyCode::D)
                        keyDPressed = true;
                    else if(keyDown->physicalCode == ui::event::PhysicalKeyCode::LShift)
                        keyLeftShiftPressed = true;
                    else if(keyDown->physicalCode == ui::event::PhysicalKeyCode::RShift)
                        keyRightShiftPressed = true;
                    else if(keyDown->physicalCode == ui::event::PhysicalKeyCode::Space)
                        keySpacePressed = true;
                }
                if(auto keyUp = dynamic_cast<const ui::event::KeyUp *>(&event))
                {
                    if(keyUp->physicalCode == ui::event::PhysicalKeyCode::W)
                        keyWPressed = false;
                    else if(keyUp->physicalCode == ui::event::PhysicalKeyCode::A)
                        keyAPressed = false;
                    else if(keyUp->physicalCode == ui::event::PhysicalKeyCode::S)
                        keySPressed = false;
                    else if(keyUp->physicalCode == ui::event::PhysicalKeyCode::D)
                        keyDPressed = false;
                    else if(keyUp->physicalCode == ui::event::PhysicalKeyCode::LShift)
                        keyLeftShiftPressed = false;
                    else if(keyUp->physicalCode == ui::event::PhysicalKeyCode::RShift)
                        keyRightShiftPressed = false;
                    else if(keyUp->physicalCode == ui::event::PhysicalKeyCode::Space)
                        keySpacePressed = false;
                }
                if(auto mouseMove = dynamic_cast<const ui::event::MouseMove *>(&event))
                {
                    auto outputSize = graphics::Driver::get().getOutputSize();
                    deltaViewTheta -= static_cast<float>(mouseMove->dx) / std::get<0>(outputSize);
                    deltaViewPhi -= static_cast<float>(mouseMove->dy) / std::get<1>(outputSize);
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
