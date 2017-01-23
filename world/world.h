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

#ifndef WORLD_WORLD_H_
#define WORLD_WORLD_H_

#include "hashlife_world.h"
#include "dimension.h"
#include "../util/constexpr_assert.h"
#include <limits>
#include <memory>
#include <mutex>
#include <condition_variable>
#include "../threading/threading.h"
#include <functional>
#include <deque>

namespace programmerjake
{
namespace voxels
{
namespace world
{
class World final
{
private:
    struct PrivateAccess final
    {
        friend class World;

    private:
        PrivateAccess() = default;
    };
    class WorkQueueItemState final
    {
    public:
        enum State
        {
            Queued,
            Finished,
            Canceled
        };

    private:
        std::mutex lock;
        std::condition_variable cond;
        State state;

    public:
        explicit WorkQueueItemState(State state = Queued) : lock(), cond(), state(state)
        {
        }
        State get()
        {
            std::unique_lock<std::mutex> lockIt(lock);
            return state;
        }
        State wait()
        {
            std::unique_lock<std::mutex> lockIt(lock);
            while(state == Queued)
                cond.wait(lockIt);
            return state;
        }
        void cancel()
        {
            std::unique_lock<std::mutex> lockIt(lock);
            constexprAssert(state == Queued);
            state = Canceled;
            cond.notify_all();
        }
        void finish()
        {
            std::unique_lock<std::mutex> lockIt(lock);
            constexprAssert(state == Queued);
            state = Finished;
            cond.notify_all();
        }
    };
    template <typename FunctionType>
    struct WorkQueueItem final
    {
        std::function<FunctionType> function;
        std::shared_ptr<WorkQueueItemState> state = std::make_shared<WorkQueueItemState>();
        explicit WorkQueueItem(std::function<FunctionType> function) : function(std::move(function))
        {
        }
    };
    struct DimensionData final
    {
        const Dimension dimension;
        std::mutex snapshotLock;
        std::shared_ptr<const HashlifeWorld::Snapshot> snapshot;
        threading::Thread moveThread;
        std::mutex moveThreadLock;
        std::condition_variable moveThreadCond;
        typedef void MoveThreadWorkQueueFunction(
            const std::shared_ptr<HashlifeWorld> &hashlifeWorld,
            block::BlockStepGlobalState &blockStepGlobalState);
        std::deque<WorkQueueItem<MoveThreadWorkQueueFunction>> moveThreadWorkQueue;
        bool moveThreadDone = false;
        bool moveThreadStarted = false;
        explicit DimensionData(Dimension dimension) : dimension(dimension)
        {
        }
    };

private:
    void moveThreadFn(std::shared_ptr<DimensionData> dimensionData) noexcept;
    static std::shared_ptr<WorkQueueItemState> scheduleOnMoveThread(
        const std::shared_ptr<DimensionData> &dimensionData,
        std::function<DimensionData::MoveThreadWorkQueueFunction> function);
    static void runOnMoveThread(const std::shared_ptr<DimensionData> &dimensionData,
                                std::function<DimensionData::MoveThreadWorkQueueFunction> function,
                                WorkQueueItemState::State &resultState);
    std::shared_ptr<DimensionData> makeDimensionData(Dimension dimension);
    std::shared_ptr<DimensionData> getOrMakeDimensionData(Dimension dimension)
    {
        std::unique_lock<std::mutex> lockIt(dimensionDataMapLock);
        auto &retval = dimensionDataMap[dimension];
        if(!retval)
            retval = makeDimensionData(dimension);
        return retval;
    }

private:
    DimensionMap<std::shared_ptr<DimensionData>> dimensionDataMap;
    std::mutex dimensionDataMapLock;

public:
    World(PrivateAccess) : dimensionDataMap()
    {
    }
    ~World();
    static std::shared_ptr<World> make()
    {
        return std::make_shared<World>(PrivateAccess());
    }
    template <typename BlocksArray>
    void setBlocks(BlocksArray &&blocksArray,
                   Position3I32 worldPosition,
                   util::Vector3I32 arrayPosition,
                   util::Vector3I32 size)
    {
        auto dimensionData = getOrMakeDimensionData(worldPosition.d);
        WorkQueueItemState::State resultState;
        runOnMoveThread(
            dimensionData,
            [&](const std::shared_ptr<HashlifeWorld> &hashlifeWorld,
                block::BlockStepGlobalState &blockStepGlobalState) noexcept
            {
                hashlifeWorld->setBlocks(
                    std::forward<BlocksArray>(blocksArray), worldPosition, arrayPosition, size);
            },
            resultState);
        constexprAssert(resultState == WorkQueueItemState::Finished);
    }
    template <typename BlocksArray>
    void getBlocks(BlocksArray &&blocksArray,
                   Position3I32 worldPosition,
                   util::Vector3I32 arrayPosition,
                   util::Vector3I32 size)
    {
        auto dimensionData = getOrMakeDimensionData(worldPosition.d);
        WorkQueueItemState::State resultState;
        runOnMoveThread(
            dimensionData,
            [&](const std::shared_ptr<HashlifeWorld> &hashlifeWorld,
                block::BlockStepGlobalState &blockStepGlobalState) noexcept
            {
                hashlifeWorld->getBlocks(
                    std::forward<BlocksArray>(blocksArray), worldPosition, arrayPosition, size);
            },
            resultState);
        constexprAssert(resultState == WorkQueueItemState::Finished);
    }
    std::shared_ptr<const HashlifeWorld::Snapshot> getDimensionSnapshot(Dimension dimension)
    {
        auto dimensionData = getOrMakeDimensionData(dimension);
        std::unique_lock<std::mutex> lockIt(dimensionData->snapshotLock);
        return dimensionData->snapshot;
    }
};
}
}
}

#endif /* WORLD_WORLD_H_ */
