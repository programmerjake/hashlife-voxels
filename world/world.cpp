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
#include "world.h"

namespace programmerjake
{
namespace voxels
{
namespace world
{
void World::moveThreadFn(std::shared_ptr<DimensionData> dimensionData) noexcept
{
    auto hashlifeWorld = HashlifeWorld::make();
    std::unique_lock<std::mutex> lockIt(dimensionData->moveThreadLock);
    while(true)
    {
        if(dimensionData->moveThreadDone)
            break;
        while(!dimensionData->moveThreadWorkQueue.empty())
        {
            auto item = std::move(dimensionData->moveThreadWorkQueue.front());
            dimensionData->moveThreadWorkQueue.pop_front();
            lockIt.unlock();
            item.function(hashlifeWorld);
            lockIt.lock();
            std::unique_lock<std::mutex> lockedSnapshot(dimensionData->snapshotLock);
            if(!hashlifeWorld->isSame(dimensionData->snapshot))
                dimensionData->snapshot = hashlifeWorld->makeSnapshot();
            lockedSnapshot.unlock();
            item.state->finish();
        }
#warning finish
        constexprAssert(false);
    }
    dimensionData->moveThreadDone = true;
    while(!dimensionData->moveThreadWorkQueue.empty())
    {
        dimensionData->moveThreadWorkQueue.front().state->cancel();
        dimensionData->moveThreadWorkQueue.pop_front();
    }
#warning finish
}

std::shared_ptr<World::WorkQueueItemState> World::scheduleOnMoveThread(
    const std::shared_ptr<DimensionData> &dimensionData,
    std::function<DimensionData::MoveThreadWorkQueueFunction> function)
{
    std::unique_lock<std::mutex> lockIt(dimensionData->moveThreadLock);
    if(dimensionData->moveThreadDone)
        return std::make_shared<WorkQueueItemState>(WorkQueueItemState::Canceled);
    if(dimensionData->moveThreadWorkQueue.empty())
        dimensionData->moveThreadCond.notify_all();
    dimensionData->moveThreadWorkQueue.push_back(
        WorkQueueItem<DimensionData::MoveThreadWorkQueueFunction>(std::move(function)));
    return dimensionData->moveThreadWorkQueue.back().state;
}

void World::runOnMoveThread(const std::shared_ptr<DimensionData> &dimensionData,
                            std::function<DimensionData::MoveThreadWorkQueueFunction> function,
                            WorkQueueItemState::State &resultState)
{
    resultState = scheduleOnMoveThread(dimensionData, std::move(function))->wait();
}

std::shared_ptr<World::DimensionData> World::makeDimensionData()
{
#warning finish
    constexprAssert(false);
}

World::~World()
{
#warning finish
    constexprAssert(false);
}
}
}
}
