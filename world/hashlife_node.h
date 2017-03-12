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

#ifndef WORLD_HASHLIFE_NODE_H_
#define WORLD_HASHLIFE_NODE_H_

#include "../block/block.h"
#include "../util/vector.h"
#include "../util/optional.h"
#include "../block/block_descriptor.h"
#include "../util/constexpr_assert.h"
#include "../util/constexpr_array.h"
#include "../util/hash.h"
#include <type_traits>
#include <list>
#include <cstdint>
#include <atomic>
#include <utility>

namespace programmerjake
{
namespace voxels
{
namespace world
{
class HashlifeNodeBase;

template <typename T, bool IsAtomic>
class HashlifeNodeReference;

template <bool IsAtomic>
struct HashlifeNodeReferenceHelper;

template <bool IsAtomic>
class HashlifeNodeReferenceBase
{
    friend class HashlifeNodeBase;
    template <typename, bool>
    friend class HashlifeNodeReference;
    template <bool>
    friend class HashlifeNodeReferenceBase;

private:
    const HashlifeNodeBase *pointer;

private:
    constexpr explicit HashlifeNodeReferenceBase(const HashlifeNodeBase *pointer) noexcept
        : pointer(pointer)
    {
    }

public:
    constexpr HashlifeNodeReferenceBase() noexcept : pointer(nullptr)
    {
    }
    constexpr HashlifeNodeReferenceBase(std::nullptr_t) noexcept : pointer(nullptr)
    {
    }
    HashlifeNodeReferenceBase(HashlifeNodeReferenceBase &&rt) noexcept : pointer(rt.pointer)
    {
        rt.pointer = nullptr;
    }
    HashlifeNodeReferenceBase(const HashlifeNodeReferenceBase &rt) noexcept : pointer(rt.pointer)
    {
        if(pointer)
            HashlifeNodeReferenceHelper<IsAtomic>::incReferenceCount(pointer);
    }
    explicit HashlifeNodeReferenceBase(const HashlifeNodeReferenceBase<!IsAtomic> &rt) noexcept
        : pointer(rt.pointer)
    {
        if(pointer)
            HashlifeNodeReferenceHelper<IsAtomic>::incReferenceCount(pointer);
    }
    explicit HashlifeNodeReferenceBase(HashlifeNodeReferenceBase<!IsAtomic> &&rt) noexcept
        : HashlifeNodeReferenceBase(HashlifeNodeReferenceBase<!IsAtomic>(std::move(rt)))
    {
    }
    HashlifeNodeReferenceBase &operator=(HashlifeNodeReferenceBase rt) noexcept
    {
        std::swap(pointer, rt.pointer);
        return *this;
    }
    ~HashlifeNodeReferenceBase()
    {
        if(pointer)
            HashlifeNodeReferenceHelper<IsAtomic>::decReferenceCountAndFreeIfNeeded(pointer);
    }
    std::size_t useCount() const noexcept
    {
        if(pointer)
            return HashlifeNodeReferenceHelper<IsAtomic>::getUseCount(pointer);
        return 0;
    }
    bool unique() const noexcept
    {
        return useCount() == 1;
    }
    template <bool IsAtomic2>
    friend bool operator==(const HashlifeNodeReferenceBase<IsAtomic> &a,
                           const HashlifeNodeReferenceBase<IsAtomic2> &b) noexcept
    {
        return a.pointer == b;
    }
    template <bool IsAtomic2>
    friend bool operator!=(const HashlifeNodeReferenceBase<IsAtomic> &a,
                           const HashlifeNodeReferenceBase<IsAtomic2> &b) noexcept
    {
        return a.pointer != b;
    }
    friend bool operator==(std::nullptr_t, const HashlifeNodeReferenceBase<IsAtomic> &v) noexcept
    {
        return v.pointer == nullptr;
    }
    friend bool operator!=(std::nullptr_t, const HashlifeNodeReferenceBase<IsAtomic> &v) noexcept
    {
        return v.pointer != nullptr;
    }
    friend bool operator==(const HashlifeNodeReferenceBase<IsAtomic> &v, std::nullptr_t) noexcept
    {
        return v.pointer == nullptr;
    }
    friend bool operator!=(const HashlifeNodeReferenceBase<IsAtomic> &v, std::nullptr_t) noexcept
    {
        return v.pointer != nullptr;
    }
    friend bool operator==(const HashlifeNodeBase *other,
                           const HashlifeNodeReferenceBase<IsAtomic> &v) noexcept
    {
        return v.pointer == other;
    }
    friend bool operator!=(const HashlifeNodeBase *other,
                           const HashlifeNodeReferenceBase<IsAtomic> &v) noexcept
    {
        return v.pointer != other;
    }
    friend bool operator==(const HashlifeNodeReferenceBase<IsAtomic> &v,
                           const HashlifeNodeBase *other) noexcept
    {
        return v.pointer == other;
    }
    friend bool operator!=(const HashlifeNodeReferenceBase<IsAtomic> &v,
                           const HashlifeNodeBase *other) noexcept
    {
        return v.pointer != other;
    }
};

template <typename T, bool IsAtomic>
class HashlifeNodeReference final : public HashlifeNodeReferenceBase<IsAtomic>
{
    template <typename, bool>
    friend class HashlifeNodeReference;
    friend class HashlifeNodeBase;
    friend class HashlifeGarbageCollectedHashtable;
    static_assert(std::is_base_of<HashlifeNodeBase, typename std::remove_const<T>::type>::value,
                  "");

public:
    constexpr HashlifeNodeReference() noexcept
    {
    }
    constexpr HashlifeNodeReference(std::nullptr_t) noexcept
        : HashlifeNodeReferenceBase<IsAtomic>(nullptr)
    {
    }
    template <typename U,
              typename = typename std::enable_if<std::is_convertible<U *, T *>::value>::type>
    HashlifeNodeReference(HashlifeNodeReference<U, IsAtomic> &&rt) noexcept
        : HashlifeNodeReferenceBase<IsAtomic>(std::move(rt))
    {
    }
    template <typename U,
              typename = typename std::enable_if<std::is_convertible<U *, T *>::value>::type>
    HashlifeNodeReference(const HashlifeNodeReference<U, IsAtomic> &rt) noexcept
        : HashlifeNodeReferenceBase<IsAtomic>(rt)
    {
    }
    template <typename U,
              bool IsAtomic2,
              typename = typename std::enable_if<std::is_convertible<U *, T *>::value
                                                 && IsAtomic != IsAtomic2>::type>
    explicit HashlifeNodeReference(HashlifeNodeReference<U, IsAtomic2> &&rt) noexcept
        : HashlifeNodeReferenceBase<IsAtomic>(std::move(rt))
    {
    }
    template <typename U,
              bool IsAtomic2,
              typename = typename std::enable_if<std::is_convertible<U *, T *>::value
                                                 && IsAtomic != IsAtomic2>::type>
    explicit HashlifeNodeReference(const HashlifeNodeReference<U, IsAtomic2> &rt) noexcept
        : HashlifeNodeReferenceBase<IsAtomic>(rt)
    {
    }

private:
    constexpr explicit HashlifeNodeReference(T *pointer) noexcept
        : HashlifeNodeReferenceBase<IsAtomic>(pointer)
    {
    }
    T *release() noexcept
    {
        T *retval = get();
        this->pointer = nullptr;
        return retval;
    }

public:
    T *get() const noexcept
    {
        return static_cast<T *>(const_cast<HashlifeNodeBase *>(this->pointer));
    }
    explicit operator bool() const noexcept
    {
        return this->pointer != nullptr;
    }
    T &operator*() const noexcept
    {
        return *operator->();
    }
    T *operator->() const noexcept
    {
        constexprAssert(this->pointer != nullptr);
        return get();
    }
    template <typename U>
    friend HashlifeNodeReference<U, IsAtomic> const_pointer_cast(
        HashlifeNodeReference<T, IsAtomic> value)
    {
        return HashlifeNodeReference<U, IsAtomic>(const_cast<U *>(value.release()));
    }
    template <typename U>
    friend HashlifeNodeReference<U, IsAtomic> static_pointer_cast(
        HashlifeNodeReference<T, IsAtomic> value)
    {
        return HashlifeNodeReference<U, IsAtomic>(static_cast<U *>(value.release()));
    }
};

struct HashlifeNonleafNode;
struct HashlifeLeafNode;
class HashlifeNodeBase
{
    friend struct HashlifeNonleafNode;
    friend struct HashlifeLeafNode;
    template <bool>
    friend class HashlifeNodeReferenceBase;
    template <bool>
    friend struct HashlifeNodeReferenceHelper;
    friend class HashlifeGarbageCollectedHashtable;

private:
    struct ReferenceCounts final
    {
        std::atomic_size_t atomicReferenceCount{1};
        std::size_t nonAtomicReferenceCount{1};
        ReferenceCounts() = default;
        ReferenceCounts &operator=(const ReferenceCounts &) = delete;
        ReferenceCounts(const ReferenceCounts &) noexcept : ReferenceCounts()
        {
        }
    };
    mutable ReferenceCounts referenceCounts;
    mutable const HashlifeNodeBase *hashNext = nullptr;

public:
    static constexpr std::int32_t levelSize = 2;
    typedef std::uint8_t LevelType;
    static constexpr LevelType maxLevel = 32 - 2;
    const LevelType level;
    const block::BlockSummary blockSummary;
    static constexpr bool isLeaf(LevelType level)
    {
        return level == 0;
    }
    bool isLeaf() const
    {
        return isLeaf(level);
    }
    static constexpr std::uint32_t getSize(LevelType level)
    {
        static_assert(levelSize == 2, "");
        return 2UL << level;
    }
    static constexpr int getLogBase2OfSize(LevelType level)
    {
        static_assert(levelSize == 2, "");
        return 1 + level;
    }
    std::uint32_t getSize() const
    {
        return getSize(level);
    }
    static constexpr std::int32_t getHalfSize(LevelType level)
    {
        static_assert(levelSize == 2, "");
        return 1L << level;
    }
    std::int32_t getHalfSize() const
    {
        return getHalfSize(level);
    }
    static constexpr std::int32_t getQuarterSize(LevelType level)
    {
        static_assert(levelSize == 2, "");
        return (constexprAssert(level >= 1), 1L << (level - 1));
    }
    std::int32_t getQuarterSize() const
    {
        return getQuarterSize(level);
    }
    static constexpr std::int32_t getEighthSize(LevelType level)
    {
        static_assert(levelSize == 2, "");
        return (constexprAssert(level >= 2), 1L << (level - 2));
    }
    std::int32_t getEighthSize() const
    {
        return getEighthSize(level);
    }
    bool isPositionInside(std::int32_t position) const
    {
        return position >= -getHalfSize() && position < getHalfSize();
    }
    bool isPositionInside(util::Vector3I32 position) const
    {
        return isPositionInside(position.x) && isPositionInside(position.y)
               && isPositionInside(position.z);
    }
    std::uint32_t getIndex(std::int32_t position) const
    {
        return (constexprAssert(isPositionInside(position)), position >= 0 ? 1 : 0);
    }
    std::int32_t getChildPosition(std::int32_t position) const
    {
        return (constexprAssert(isPositionInside(position)),
                position >= 0 ? position - getQuarterSize() : position + getQuarterSize());
    }
    util::Vector3U32 getIndex(util::Vector3I32 position) const
    {
        return util::Vector3U32(getIndex(position.x), getIndex(position.y), getIndex(position.z));
    }
    util::Vector3I32 getChildPosition(util::Vector3I32 position) const
    {
        return util::Vector3I32(getChildPosition(position.x),
                                getChildPosition(position.y),
                                getChildPosition(position.z));
    }
    block::Block get(util::Vector3I32 position) const;
    const HashlifeNodeBase *get(util::Vector3I32 position, LevelType returnedLevel) const;
    HashlifeNodeBase *get(util::Vector3I32 position, LevelType returnedLevel)
    {
        return const_cast<HashlifeNodeBase *>(
            static_cast<const HashlifeNodeBase *>(this)->get(position, returnedLevel));
    }
    HashlifeNodeReference<HashlifeNodeBase, false> duplicate() const &;
    HashlifeNodeReference<HashlifeNodeBase, false> duplicate() && ;
    template <bool IsAtomic>
    HashlifeNodeReference<const HashlifeNodeBase, IsAtomic> referenceFromThis() const noexcept
    {
        HashlifeNodeReferenceHelper<IsAtomic>::incReferenceCount(this);
        return HashlifeNodeReference<const HashlifeNodeBase, IsAtomic>(this);
    }
    template <bool IsAtomic>
    HashlifeNodeReference<HashlifeNodeBase, IsAtomic> referenceFromThis() noexcept
    {
        HashlifeNodeReferenceHelper<IsAtomic>::incReferenceCount(this);
        return HashlifeNodeReference<HashlifeNodeBase, IsAtomic>(this);
    }
    friend HashlifeLeafNode *getAsLeaf(HashlifeNodeBase *node) noexcept;
    friend HashlifeNonleafNode *getAsNonleaf(HashlifeNodeBase *node) noexcept;
    friend const HashlifeLeafNode *getAsLeaf(const HashlifeNodeBase *node) noexcept;
    friend const HashlifeNonleafNode *getAsNonleaf(const HashlifeNodeBase *node) noexcept;
    friend HashlifeLeafNode &getAsLeaf(HashlifeNodeBase &node) noexcept
    {
        return *getAsLeaf(&node);
    }
    friend HashlifeNonleafNode &getAsNonleaf(HashlifeNodeBase &node) noexcept
    {
        return *getAsNonleaf(&node);
    }
    friend const HashlifeLeafNode &getAsLeaf(const HashlifeNodeBase &node) noexcept
    {
        return *getAsLeaf(&node);
    }
    friend const HashlifeNonleafNode &getAsNonleaf(const HashlifeNodeBase &node) noexcept
    {
        return *getAsNonleaf(&node);
    }
    static void free(HashlifeNodeBase *node) noexcept;

private:
    HashlifeNodeBase(LevelType level, const block::BlockSummary &blockSummary)
        : level((constexprAssert(level <= maxLevel), level))
    {
    }
};

template <>
struct HashlifeNodeReferenceHelper<true> final // atomic
{
    static void incReferenceCount(const HashlifeNodeBase *pointer) noexcept
    {
        pointer->referenceCounts.atomicReferenceCount.fetch_add(1, std::memory_order_relaxed);
    }
    static void decReferenceCountAndFreeIfNeeded(const HashlifeNodeBase *pointer) noexcept
    {
        if(pointer->referenceCounts.atomicReferenceCount.fetch_sub(1, std::memory_order_acq_rel)
           == 1)
        {
            HashlifeNodeBase::free(const_cast<HashlifeNodeBase *>(pointer));
        }
    }
    static std::size_t getUseCount(const HashlifeNodeBase *pointer) noexcept
    {
        return pointer->referenceCounts.atomicReferenceCount.load(std::memory_order_relaxed);
    }
};

template <>
struct HashlifeNodeReferenceHelper<false> final // non-atomic
{
    static void incReferenceCount(const HashlifeNodeBase *pointer) noexcept
    {
        pointer->referenceCounts.nonAtomicReferenceCount++;
    }
    static void decReferenceCountAndFreeIfNeeded(const HashlifeNodeBase *pointer) noexcept
    {
        if(pointer->referenceCounts.nonAtomicReferenceCount-- == 1)
            HashlifeNodeReferenceHelper<true>::decReferenceCountAndFreeIfNeeded(pointer);
    }
    static std::size_t getUseCount(const HashlifeNodeBase *pointer) noexcept
    {
        return pointer->referenceCounts.nonAtomicReferenceCount;
    }
};

class HashlifeNonleafNode final : public HashlifeNodeBase
{
public:
    struct FutureState final
    {
        static constexpr std::uint32_t getStepSizeInGenerations(LevelType level)
        {
            return (constexprAssert(level >= 1),
                    static_cast<LevelType>(level - 1)
                            > block::BlockStepGlobalState::log2OfStepSizeInGenerations ?
                        block::BlockStepGlobalState::stepSizeInGenerations :
                        1UL << (level - 1));
        }
        HashlifeNodeReference<const HashlifeNodeBase, false> node;
        block::BlockStepGlobalState globalState;
        typedef util::Array<util::Array<util::Array<block::BlockStepExtraActions, levelSize>,
                                        levelSize>,
                            levelSize> ActionsArray;
        ActionsArray actions;
        constexpr FutureState() : node(nullptr), globalState(), actions()
        {
        }
        FutureState(HashlifeNodeReference<const HashlifeNodeBase, false> node,
                    block::BlockStepGlobalState globalState,
                    ActionsArray actions)
            : node(std::move(node)),
              globalState(std::move(globalState)),
              actions(std::move(actions))
        {
        }
        explicit FutureState(block::BlockStepGlobalState globalState)
            : node(nullptr), globalState(std::move(globalState)), actions()
        {
        }
    };
    typedef util::
        Array<util::Array<util::Array<HashlifeNodeReference<const HashlifeNodeBase, false>,
                                      levelSize>,
                          levelSize>,
              levelSize> ChildNodesArray;

private:
    const ChildNodesArray childNodes;

public:
    const HashlifeNodeReference<const HashlifeNodeBase, false> &getChildNode(
        util::Vector3U32 index) const
    {
        return (constexprAssert(!isLeaf()), childNodes[index.x][index.y][index.z]);
    }
    const HashlifeNodeReference<const HashlifeNodeBase, false> &getChildNode(
        util::Vector3I32 index) const
    {
        return getChildNode(util::Vector3U32(index));
    }
    mutable FutureState futureState; // ignored for operator == and hash
    HashlifeNonleafNode(HashlifeNodeReference<const HashlifeNodeBase, false> nxnynz,
                        HashlifeNodeReference<const HashlifeNodeBase, false> nxnypz,
                        HashlifeNodeReference<const HashlifeNodeBase, false> nxpynz,
                        HashlifeNodeReference<const HashlifeNodeBase, false> nxpypz,
                        HashlifeNodeReference<const HashlifeNodeBase, false> pxnynz,
                        HashlifeNodeReference<const HashlifeNodeBase, false> pxnypz,
                        HashlifeNodeReference<const HashlifeNodeBase, false> pxpynz,
                        HashlifeNodeReference<const HashlifeNodeBase, false> pxpypz)
        : HashlifeNodeBase((constexprAssert(nxnynz), nxnynz->level + 1),
                           nxnynz->blockSummary + nxnypz->blockSummary + nxpynz->blockSummary
                               + nxpypz->blockSummary + pxnynz->blockSummary + pxnypz->blockSummary
                               + pxpynz->blockSummary + pxpypz->blockSummary),
          childNodes{
              (constexprAssert(nxnynz && nxnynz->level + 1 == level), std::move(nxnynz)),
              (constexprAssert(nxnypz && nxnypz->level + 1 == level), std::move(nxnypz)),
              (constexprAssert(nxpynz && nxpynz->level + 1 == level), std::move(nxpynz)),
              (constexprAssert(nxpypz && nxpypz->level + 1 == level), std::move(nxpypz)),
              (constexprAssert(pxnynz && pxnynz->level + 1 == level), std::move(pxnynz)),
              (constexprAssert(pxnypz && pxnypz->level + 1 == level), std::move(pxnypz)),
              (constexprAssert(pxpynz && pxpynz->level + 1 == level), std::move(pxpynz)),
              (constexprAssert(pxpypz && pxpypz->level + 1 == level), std::move(pxpypz)),
          },
          futureState()
    {
        static_assert(levelSize == 2, "");
    }
    explicit HashlifeNonleafNode(ChildNodesArray childNodes)
        : HashlifeNonleafNode(std::move(childNodes[0][0][0]),
                              std::move(childNodes[0][0][1]),
                              std::move(childNodes[0][1][0]),
                              std::move(childNodes[0][1][1]),
                              std::move(childNodes[1][0][0]),
                              std::move(childNodes[1][0][1]),
                              std::move(childNodes[1][1][0]),
                              std::move(childNodes[1][1][1]))
    {
        static_assert(levelSize == 2, "");
    }
    static std::size_t hashNode(const ChildNodesArray &childNodes)
    {
        util::FastHasher hasher;
        for(auto &i : childNodes)
            for(auto &j : i)
                for(auto childNode : j)
                    hasher = next(hasher, childNode.get());
        return finish(hasher);
    }
    static bool equalsNode(const HashlifeNodeBase *node, const ChildNodesArray &childNodes)
    {
        return !node->isLeaf() && getAsNonleaf(node)->childNodes == childNodes;
    }
};

class HashlifeLeafNode final : public HashlifeNodeBase
{
public:
    typedef util::Array<util::Array<util::Array<block::Block, levelSize>, levelSize>, levelSize>
        BlocksArray;

private:
    const BlocksArray blocks;

public:
    block::Block getBlock(util::Vector3I32 index) const
    {
        return getBlock(util::Vector3U32(index));
    }
    block::Block getBlock(util::Vector3U32 index) const
    {
        return (constexprAssert(isLeaf()), blocks[index.x][index.y][index.z]);
    }
    HashlifeLeafNode(block::Block nxnynz,
                     block::Block nxnypz,
                     block::Block nxpynz,
                     block::Block nxpypz,
                     block::Block pxnynz,
                     block::Block pxnypz,
                     block::Block pxpynz,
                     block::Block pxpypz,
                     const block::BlockSummary &blockSummary)
        : HashlifeNodeBase(0, blockSummary),
          blocks{
              nxnynz, nxnypz, nxpynz, nxpypz, pxnynz, pxnypz, pxpynz, pxpypz,
          }
    {
        static_assert(levelSize == 2, "");
    }
    HashlifeLeafNode(block::Block nxnynz,
                     block::Block nxnypz,
                     block::Block nxpynz,
                     block::Block nxpypz,
                     block::Block pxnynz,
                     block::Block pxnypz,
                     block::Block pxpynz,
                     block::Block pxpypz)
        : HashlifeLeafNode(nxnynz,
                           nxnypz,
                           nxpynz,
                           nxpypz,
                           pxnynz,
                           pxnypz,
                           pxpynz,
                           pxpypz,
                           (block::BlockDescriptor::getBlockSummary(nxnynz.getBlockKind())
                            + block::BlockDescriptor::getBlockSummary(nxnypz.getBlockKind())
                            + block::BlockDescriptor::getBlockSummary(nxpynz.getBlockKind())
                            + block::BlockDescriptor::getBlockSummary(nxpypz.getBlockKind())
                            + block::BlockDescriptor::getBlockSummary(pxnynz.getBlockKind())
                            + block::BlockDescriptor::getBlockSummary(pxnypz.getBlockKind())
                            + block::BlockDescriptor::getBlockSummary(pxpynz.getBlockKind())
                            + block::BlockDescriptor::getBlockSummary(pxpypz.getBlockKind())))
    {
    }
    HashlifeLeafNode(const BlocksArray &blocks, const block::BlockSummary &blockSummary)
        : HashlifeNodeBase(0, blockSummary), blocks(blocks)
    {
    }
    HashlifeLeafNode(const BlocksArray &blocks)
        : HashlifeLeafNode(blocks[0][0][0],
                           blocks[0][0][1],
                           blocks[0][1][0],
                           blocks[0][1][1],
                           blocks[1][0][0],
                           blocks[1][0][1],
                           blocks[1][1][0],
                           blocks[1][1][1])
    {
    }
    static std::size_t hashNode(const BlocksArray &blocks)
    {
        util::FastHasher hasher;
        for(auto &i : blocks)
            for(auto &j : i)
                for(auto &block : j)
                    hasher = next(hasher, block.value);
        return finish(hasher);
    }
    static bool equalsNode(const HashlifeNodeBase *node, const BlocksArray &blocks)
    {
        return node->isLeaf() && getAsLeaf(node)->blocks == blocks;
    }
};

inline HashlifeLeafNode *getAsLeaf(HashlifeNodeBase *node) noexcept
{
    return (constexprAssert(node->isLeaf()), static_cast<HashlifeLeafNode *>(node));
}

inline HashlifeNonleafNode *getAsNonleaf(HashlifeNodeBase *node) noexcept
{
    return (constexprAssert(!node->isLeaf()), static_cast<HashlifeNonleafNode *>(node));
}

inline const HashlifeLeafNode *getAsLeaf(const HashlifeNodeBase *node) noexcept
{
    return (constexprAssert(node->isLeaf()), static_cast<const HashlifeLeafNode *>(node));
}

inline const HashlifeNonleafNode *getAsNonleaf(const HashlifeNodeBase *node) noexcept
{
    return (constexprAssert(!node->isLeaf()), static_cast<const HashlifeNonleafNode *>(node));
}

inline block::Block HashlifeNodeBase::get(util::Vector3I32 position) const
{
    return isLeaf() ? getAsLeaf(this)->getBlock(getIndex(position)) :
                      getAsNonleaf(this)
                          ->getChildNode(getIndex(position))
                          ->get(getChildPosition(position));
}

inline const HashlifeNodeBase *HashlifeNodeBase::get(util::Vector3I32 position,
                                                     LevelType returnedLevel) const
{
    return (constexprAssert(level >= returnedLevel),
            level == returnedLevel ? this : getAsNonleaf(this)
                                                ->getChildNode(getIndex(position))
                                                ->get(getChildPosition(position), returnedLevel));
}

inline HashlifeNodeReference<HashlifeNodeBase, false> HashlifeNodeBase::duplicate() const &
{
    if(isLeaf())
        return HashlifeNodeReference<HashlifeNodeBase, false>(
            new HashlifeLeafNode(*getAsLeaf(this)));
    else
        return HashlifeNodeReference<HashlifeNodeBase, false>(
            new HashlifeNonleafNode(*getAsNonleaf(this)));
}

inline HashlifeNodeReference<HashlifeNodeBase, false> HashlifeNodeBase::duplicate() &&
{
    if(isLeaf())
        return HashlifeNodeReference<HashlifeNodeBase, false>(
            new HashlifeLeafNode(std::move(*getAsLeaf(this))));
    else
        return HashlifeNodeReference<HashlifeNodeBase, false>(
            new HashlifeNonleafNode(std::move(*getAsNonleaf(this))));
}

inline void HashlifeNodeBase::free(HashlifeNodeBase *node) noexcept
{
    if(node->isLeaf())
        delete getAsLeaf(node);
    else
        delete getAsNonleaf(node);
}
}
}
}

#endif /* WORLD_HASHLIFE_NODE_H_ */
