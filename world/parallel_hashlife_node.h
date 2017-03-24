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
#include "parallel_hashlife_memory_manager.h"
#ifndef WORLD_PARALLEL_HASHLIFE_NODE_H_
#define WORLD_PARALLEL_HASHLIFE_NODE_H_

#include "../block/block.h"
#include "../util/vector.h"
#include "../util/optional.h"
#include "../block/block_descriptor.h"
#include "../util/constexpr_assert.h"
#include "../util/constexpr_array.h"
#include "../util/hash.h"
#include "../util/enum.h"
#include <type_traits>
#include <list>
#include <cstdint>
#include <atomic>
#include <utility>
#include <cstring>
#include <memory>

namespace programmerjake
{
namespace voxels
{
namespace world
{
namespace parallel_hashlife
{
enum class StepKind
{
    General,
    DEFINE_ENUM_LIMITS(General, General)
};

template <std::size_t Level>
struct Node;

template <std::size_t Level, typename DerivedType>
struct NodeBase : public MemoryManagerNodeBase
{
    static_assert(std::is_base_of<NodeBase, DerivedType>::value, "");
    static_assert(std::is_same<Node<Level>, DerivedType>::value, "");
    static constexpr std::size_t level = Level;
    static constexpr std::uint32_t halfSize = 1UL << level;
    static constexpr std::uint32_t size = halfSize * 2;
    static_assert(size != 0, "level too big"); // size wraps around to 0
    static constexpr util::Vector3I32 positionOffset = util::Vector3I32(halfSize);
    static constexpr bool isLeaf = level == 0;
    typedef typename std::conditional<isLeaf, block::Block::ValueType, Node<level - 1> *>::type
        KeyElement;
    typedef util::Array<util::Array<util::Array<KeyElement, 2>, 2>, 2> Key;
    static constexpr util::Vector3U32 getKeyIndicesFromOffsetPosition(
        util::Vector3U32 offsetPosition) noexcept
    {
        return util::Vector3U32((offsetPosition.x & halfSize) != 0,
                                (offsetPosition.y & halfSize) != 0,
                                (offsetPosition.z & halfSize) != 0);
    }
    static constexpr util::Vector3U32 getChildOffsetPositionFromOffsetPosition(
        util::Vector3U32 offsetPosition) noexcept
    {
        return util::Vector3U32(offsetPosition >> util::Vector3U32(1));
    }
    Key key;
    block::BlockSummary blockSummary;
    constexpr NodeBase(const Key &key, const block::BlockSummary &blockSummary) noexcept
        : MemoryManagerNodeBase(static_cast<Node<Level> *>(nullptr)),
          key(key),
          blockSummary(blockSummary)
    {
    }
    std::size_t keyHash() const noexcept
    {
        return keyHash(key);
    }
    friend std::size_t keyHash(const Key &key) noexcept
    {
        util::FastHasher hasher;
        for(auto &i : key)
            for(auto &j : i)
                for(auto &value : j)
                    hasher = next(hasher, value);
        return finish(hasher);
    }
    friend bool keyEqual(const Key &a, const Key &b) noexcept
    {
        return a == b;
    }
    bool keyEqual(const NodeBase &rt) const noexcept
    {
        return keyEqual(key, rt.key);
    }
};

template <std::size_t Level, typename DerivedType>
constexpr std::size_t NodeBase<Level, DerivedType>::level;

template <std::size_t Level, typename DerivedType>
constexpr std::uint32_t NodeBase<Level, DerivedType>::halfSize;

template <std::size_t Level, typename DerivedType>
constexpr std::uint32_t NodeBase<Level, DerivedType>::size;

template <std::size_t Level, typename DerivedType>
constexpr util::Vector3I32 NodeBase<Level, DerivedType>::positionOffset;

template <std::size_t Level, typename DerivedType>
constexpr bool NodeBase<Level, DerivedType>::isLeaf;

template <std::size_t Level>
struct Node final : public NodeBase<Level, Node>
{
    static constexpr std::uint32_t quarterSize = halfSize / 2U;
    static_assert(quarterSize != 0, "level too small");
    using NodeBase<Level, Node>::NodeBase;
    struct Actions final
    {
        util::EnumArray<util::Array<util::Array<util::Array<block::BlockStepExtraActions, 2>, 2>,
                                    2>,
                        StepKind> actions;
    };
    struct NextState final
    {
        KeyElement next = nullptr;
    };
    std::unique_ptr<Actions> actions;
    util::EnumArray<NextState, StepKind> nextStates;
    template <typename Fn>
    bool trace(Fn &&fn) const noexcept(noexcept(fn(static_cast<KeyElement>(nullptr)) ? 0 : 0))
    {
        for(auto &i : key)
        {
            for(auto &j : i)
            {
                for(auto &value : j)
                {
                    if(fn(value))
                        continue;
                    return false;
                }
            }
        }
        for(auto &value : nextStates)
        {
            if(fn(value))
                continue;
            return false;
        }
        return true;
    }
};

template <>
struct Node<0> final : public NodeBase<0, Node>
{
    // no quarterSize because it is too small
    using NodeBase<0, Node>::NodeBase;
    template <typename Fn>
    bool trace(Fn &&fn) const noexcept(noexcept(fn(static_cast<KeyElement>(0)) ? 0 : 0))
    {
        for(auto &i : key)
        {
            for(auto &j : i)
            {
                for(auto &value : j)
                {
                    if(fn(value))
                        continue;
                    return false;
                }
            }
        }
        return true;
    }
};

#if 1
#warning finish
#else
#error finish
template <std::size_t OutputLevel = 0, typename OutputArray, std::size_t InputLevel>
void getElements(OutputArray &&outputArray, util::Vector3I32 origin) noexcept(noexcept(
    outputArray[static_cast<std::int32_t>(0)][static_cast<std::int32_t>(
        0)][static_cast<std::int32_t>(0)] = static_cast<typename Node<OutputLevel>::KeyElement>(0)))
{
}
#endif
}
}
}
}

#endif /* WORLD_PARALLEL_HASHLIFE_NODE_H_ */
