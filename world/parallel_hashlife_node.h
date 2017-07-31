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
#include "../util/integer_sequence.h"
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
template <std::size_t Level, typename DerivedType>
struct NodeBase;
class MemoryManager;

constexpr std::size_t MaxLevel = 32 - 2;
constexpr std::size_t LevelCount = MaxLevel + 1;

struct MemoryManagerNodeBase
{
    bool gcMark : 1;
    constexpr MemoryManagerNodeBase() noexcept : gcMark(false)
    {
    }
};

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
        return util::Vector3U32(offsetPosition & util::Vector3U32(halfSize - 1));
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
struct Node final : public NodeBase<Level, Node<Level>>
{
    using NodeBase<Level, Node>::halfSize;
    using NodeBase<Level, Node>::key;
    using typename NodeBase<Level, Node>::KeyElement;
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
            if(fn(value.next))
                continue;
            return false;
        }
        return true;
    }
};

template <>
struct Node<0> final : public NodeBase<0, Node<0>>
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

class DynamicNonowningNodeReference final
{
private:
    MemoryManagerNodeBase *node;
    std::uint8_t nodeLevel;

public:
    constexpr DynamicNonowningNodeReference() noexcept : node(nullptr), nodeLevel(0)
    {
    }
    constexpr DynamicNonowningNodeReference(std::nullptr_t) noexcept : node(nullptr), nodeLevel(0)
    {
    }
    template <std::size_t Level>
    constexpr DynamicNonowningNodeReference(Node<Level> *node) noexcept
        : node(node),
          nodeLevel(node ? Level : 0)
    {
        static_assert(Level <= MaxLevel, "");
    }
    constexpr explicit DynamicNonowningNodeReference(MemoryManagerNodeBase *node,
                                                     std::size_t nodeLevel) noexcept
        : node(node),
          nodeLevel(node ? (constexprAssert(nodeLevel <= MaxLevel), nodeLevel) : 0)
    {
    }
    constexpr std::size_t level() const noexcept
    {
        return node ? nodeLevel : static_cast<std::size_t>(-1);
    }
    constexpr MemoryManagerNodeBase *get() const noexcept
    {
        return node;
    }
    constexpr explicit operator bool() const noexcept
    {
        return node != nullptr;
    }
    friend constexpr bool operator==(const DynamicNonowningNodeReference &a,
                                     const DynamicNonowningNodeReference &b) noexcept
    {
        return a.node == b.node;
    }
    friend constexpr bool operator!=(const DynamicNonowningNodeReference &a,
                                     const DynamicNonowningNodeReference &b) noexcept
    {
        return a.node != b.node;
    }
    friend constexpr bool operator==(const DynamicNonowningNodeReference &a,
                                     std::nullptr_t) noexcept
    {
        return a.node == nullptr;
    }
    friend constexpr bool operator!=(const DynamicNonowningNodeReference &a,
                                     std::nullptr_t) noexcept
    {
        return a.node != nullptr;
    }
    friend constexpr bool operator==(std::nullptr_t,
                                     const DynamicNonowningNodeReference &b) noexcept
    {
        return nullptr == b.node;
    }
    friend constexpr bool operator!=(std::nullptr_t,
                                     const DynamicNonowningNodeReference &b) noexcept
    {
        return nullptr != b.node;
    }

private:
    template <typename Fn>
    static constexpr bool dispatchIsNoexcept(Node<0> *) noexcept
    {
        return noexcept(std::declval<Fn>()(static_cast<Node<0> *>(nullptr)))
               && noexcept(std::declval<Fn>()(nullptr));
    }
    template <typename Fn, std::size_t Level>
    static constexpr bool dispatchIsNoexcept(Node<Level> *) noexcept
    {
        return noexcept(std::declval<Fn>()(static_cast<Node<Level> *>(nullptr)))
               && dispatchIsNoexcept<Fn>(static_cast<Node<Level - 1> *>(nullptr));
    }
    template <typename Fn, std::size_t Level>
    static void dispatchHelper2(MemoryManagerNodeBase *node, Fn &&fn)
    {
        std::forward<Fn>(fn)(static_cast<Node<Level> *>(node));
    }
    template <typename Fn, std::size_t... Levels>
    void dispatchHelper(Fn &&fn, util::IntegerSequence<std::size_t, Levels...>) const
        noexcept(dispatchIsNoexcept<Fn>(static_cast<Node<MaxLevel> *>(nullptr)))
    {
        if(node == nullptr)
        {
            std::forward<Fn>(fn)(nullptr);
        }
        else
        {
            typedef void (*FnPtrType)(MemoryManagerNodeBase *node, Fn &&fn);
            static constexpr FnPtrType functions[] = {dispatchHelper2<Fn, Levels>...};
            functions[level()](node, std::forward<Fn>(fn));
        }
    }

public:
    template <typename Fn>
    void dispatch(Fn &&fn) const
        noexcept(dispatchIsNoexcept<Fn>(static_cast<Node<MaxLevel> *>(nullptr)))
    {
        dispatchHelper(std::forward<Fn>(fn), util::MakeIndexSequence<LevelCount>());
    }
};

template <std::size_t OutputLevel, typename Fn, std::size_t InputLevel>
struct ForEachElementHelper;

template <std::size_t Level, typename Fn>
struct ForEachElementHelper<Level, Fn, Level>
{
    static constexpr bool isNoexcept =
        noexcept(std::declval<Fn>()(std::declval<const util::Vector3I32 &>(),
                                    std::declval<const typename Node<Level>::KeyElement &>()));
    static void forEachElement(Fn &&fn,
                               Node<Level> *node,
                               util::Vector3U32 startPositionOffseted,
                               util::Vector3U32 endPositionOffseted,
                               util::Vector3U32 outputBasePosition) noexcept(isNoexcept)
    {
        constexpr std::uint32_t nodeSize = Node<Level>::size;
        constexpr std::uint32_t nodeHalfSize = Node<Level>::halfSize;
        for(util::Vector3U32 offsetPosition(0); offsetPosition.x < nodeSize;
            offsetPosition.x += nodeHalfSize)
        {
            if(offsetPosition.x < startPositionOffseted.x
               || offsetPosition.x >= endPositionOffseted.x)
                continue;
            for(offsetPosition.y = 0; offsetPosition.y < nodeSize; offsetPosition.y += nodeHalfSize)
            {
                if(offsetPosition.y < startPositionOffseted.y
                   || offsetPosition.y >= endPositionOffseted.y)
                    continue;
                for(offsetPosition.z = 0; offsetPosition.z < nodeSize;
                    offsetPosition.z += nodeHalfSize)
                {
                    if(offsetPosition.z < startPositionOffseted.z
                       || offsetPosition.z >= endPositionOffseted.z)
                        continue;
                    const auto constPosition =
                        static_cast<util::Vector3I32>(offsetPosition + outputBasePosition)
                        - Node<Level>::positionOffset;
                    util::Vector3U32 keyIndices =
                        Node<Level>::getKeyIndicesFromOffsetPosition(offsetPosition);
                    const auto &constElement = node->key[keyIndices.x][keyIndices.y][keyIndices.z];
                    std::forward<Fn>(fn)(constPosition, constElement);
                }
            }
        }
    }
};

template <std::size_t OutputLevel, typename Fn, std::size_t InputLevel>
struct ForEachElementHelper
{
    static_assert(OutputLevel < InputLevel, "");
    static constexpr bool isNoexcept = noexcept(
        std::declval<Fn>()(std::declval<const util::Vector3I32 &>(),
                           std::declval<const typename Node<OutputLevel>::KeyElement &>()));
    static void forEachElement(Fn &&fn,
                               Node<InputLevel> *node,
                               util::Vector3U32 startPositionOffseted,
                               util::Vector3U32 endPositionOffseted,
                               util::Vector3U32 outputBasePosition) noexcept(isNoexcept)
    {
        constexpr std::uint32_t nodeSize = Node<InputLevel>::size;
        constexpr std::uint32_t nodeHalfSize = Node<InputLevel>::halfSize;
        for(util::Vector3U32 offsetPosition(0); offsetPosition.x < nodeSize;
            offsetPosition.x += nodeHalfSize)
        {
            if(offsetPosition.x < startPositionOffseted.x
               || offsetPosition.x >= endPositionOffseted.x)
                continue;
            for(offsetPosition.y = 0; offsetPosition.y < nodeSize; offsetPosition.y += nodeHalfSize)
            {
                if(offsetPosition.y < startPositionOffseted.y
                   || offsetPosition.y >= endPositionOffseted.y)
                    continue;
                for(offsetPosition.z = 0; offsetPosition.z < nodeSize;
                    offsetPosition.z += nodeHalfSize)
                {
                    if(offsetPosition.z < startPositionOffseted.z
                       || offsetPosition.z >= endPositionOffseted.z)
                        continue;
                    util::Vector3U32 keyIndices =
                        Node<InputLevel>::getKeyIndicesFromOffsetPosition(offsetPosition);
                    auto *child = node->key[keyIndices.x][keyIndices.y][keyIndices.z];
                    auto childStartPositionOffseted =
                        max(startPositionOffseted, offsetPosition) - offsetPosition;
                    auto childEndPositionOffseted = endPositionOffseted - offsetPosition;
                    ForEachElementHelper<OutputLevel, Fn, InputLevel - 1>::forEachElement(
                        std::forward<Fn>(fn),
                        child,
                        childStartPositionOffseted,
                        childEndPositionOffseted,
                        outputBasePosition + offsetPosition);
                }
            }
        }
    }
};

template <std::size_t OutputLevel,
          bool IncludeAllOutputElements = true,
          typename Fn,
          std::size_t InputLevel>
void forEachElement(Fn &&fn,
                    Node<InputLevel> *node,
                    util::Vector3I32 startPosition,
                    util::Vector3I32 endPosition) //
    noexcept(ForEachElementHelper<OutputLevel, Fn, InputLevel>::isNoexcept)
{
    startPosition =
        static_cast<util::Vector3I32>(static_cast<util::Vector3U32>(startPosition)
                                      - static_cast<util::Vector3U32>(startPosition)
                                            % util::Vector3U32(Node<OutputLevel>::halfSize));
    endPosition =
        static_cast<util::Vector3I32>(static_cast<util::Vector3U32>(endPosition)
                                      - static_cast<util::Vector3U32>(endPosition)
                                            % util::Vector3U32(Node<OutputLevel>::halfSize));
    constexpr std::int32_t inputNodeHalfSize = Node<InputLevel>::halfSize;
    constexpr std::int32_t outputNodeHalfSize = Node<OutputLevel>::halfSize;
    if(IncludeAllOutputElements)
    {
        for(util::Vector3I32 position = startPosition; position.x < endPosition.x;
            position.x += outputNodeHalfSize)
        {
            if(position.x >= -inputNodeHalfSize && position.x < inputNodeHalfSize)
            {
                position.x = inputNodeHalfSize;
                if(position.x >= endPosition.x)
                    break;
            }
            for(position.y = startPosition.y; position.y < endPosition.y;
                position.y += outputNodeHalfSize)
            {
                if(position.y >= -inputNodeHalfSize && position.y < inputNodeHalfSize)
                {
                    position.y = inputNodeHalfSize;
                    if(position.y >= endPosition.y)
                        break;
                }
                for(position.z = startPosition.z; position.z < endPosition.z;
                    position.z += outputNodeHalfSize)
                {
                    if(position.z >= -inputNodeHalfSize && position.z < inputNodeHalfSize)
                    {
                        position.z = inputNodeHalfSize;
                        if(position.z >= endPosition.z)
                            break;
                    }
                    const auto constPosition = position;
                    const typename Node<OutputLevel>::KeyElement constElement;
                    std::forward<Fn>(fn)(constPosition, constElement);
                }
            }
        }
    }
    startPosition = max(startPosition, util::Vector3I32(-inputNodeHalfSize));
    endPosition = min(endPosition, util::Vector3I32(inputNodeHalfSize));
    if(startPosition.x >= endPosition.x || startPosition.y >= endPosition.y
       || startPosition.z >= endPosition.z)
        return;
    auto startPositionOffseted =
        static_cast<util::Vector3U32>(startPosition + Node<InputLevel>::positionOffset);
    auto endPositionOffseted =
        static_cast<util::Vector3U32>(endPosition + Node<InputLevel>::positionOffset);
    ForEachElementHelper<OutputLevel, Fn, InputLevel>::forEachElement(std::forward<Fn>(fn),
                                                                      node,
                                                                      startPositionOffseted,
                                                                      endPositionOffseted,
                                                                      util::Vector3U32(0));
}

template <std::size_t OutputLevel,
          bool SetAllOutputElements = true,
          typename OutputArray,
          std::size_t InputLevel>
void getElements(OutputArray &&outputArray,
                 util::Vector3U32 outputSize,
                 util::Vector3U32 outputStartIndex,
                 Node<InputLevel> *inputNode,
                 util::Vector3I32 inputStartPosition) //
    noexcept(noexcept(std::declval<OutputArray>()[static_cast<std::int32_t>(
                          0)][static_cast<std::int32_t>(0)][static_cast<std::int32_t>(0)] =
                          std::declval<const typename Node<OutputLevel>::KeyElement &>()))
{
    constexpr bool isNoexcept =
        noexcept(std::declval<OutputArray>()[static_cast<std::int32_t>(
                     0)][static_cast<std::int32_t>(0)][static_cast<std::int32_t>(0)] =
                     std::declval<const typename Node<OutputLevel>::KeyElement &>());
    forEachElement<OutputLevel, SetAllOutputElements>(
        [&outputArray, outputStartIndex, inputStartPosition ](
            util::Vector3I32 position,
            const typename Node<OutputLevel>::KeyElement &keyElement) noexcept(isNoexcept) //
        {
            auto offsetPosition = static_cast<util::Vector3U32>(position - inputStartPosition);
            const auto index =
                offsetPosition / util::Vector3U32(Node<OutputLevel>::halfSize) + outputStartIndex;
            std::forward<OutputArray>(outputArray)[index.x][index.y][index.z] = keyElement;
        },
        inputStartPosition,
        inputStartPosition
            + static_cast<util::Vector3I32>(outputSize)
                  * util::Vector3I32(Node<OutputLevel>::halfSize));
}
}
}
}
}

#endif /* WORLD_PARALLEL_HASHLIFE_NODE_H_ */
