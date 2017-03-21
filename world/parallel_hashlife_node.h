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
enum class ParallelHashlifeStepKind
{
    General,
    Lighting,
    RedstoneWire,
    Redstone,
    DEFINE_ENUM_LIMITS(General, Redstone)
};

template <std::size_t Level>
struct ParallelHashlifeNode final
{
    static constexpr std::size_t level = Level;
    typedef util::Array<util::Array<util::Array<ParallelHashlifeNode<level - 1> *, 2>, 2>, 2> Key;
    Key key;
    constexpr ParallelHashlifeNode(const Key &key) noexcept : key(key)
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
                for(auto *value : j)
                    hasher = next(hasher, value);
        return finish(hasher);
    }
    friend bool keyEqual(const Key &a, const Key &b) noexcept
    {
        return a == b;
    }
    bool keyEqual(const ParallelHashlifeNode &rt) const noexcept
    {
        return keyEqual(key, rt.key);
    }
    struct Actions final
    {
        util::EnumArray<util::Array<util::Array<util::Array<block::BlockStepExtraActions, 2>, 2>,
                                    2>,
                        ParallelHashlifeStepKind> actions;
    };
    struct NextState final
    {
        ParallelHashlifeNode<level - 1> *next = nullptr;
    };
    std::unique_ptr<Actions> actions;
    util::EnumArray<NextState, ParallelHashlifeStepKind> nextStates;
};

template <>
struct ParallelHashlifeNode<0> final
{
    static constexpr std::size_t level = 0;
    typedef util::Array<util::Array<util::Array<block::Block::ValueType, 2>, 2>, 2> Key;
    Key key;
    constexpr ParallelHashlifeNode(const Key &key) noexcept : key(key)
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
    bool keyEqual(const ParallelHashlifeNode &rt) const noexcept
    {
        return keyEqual(key, rt.key);
    }
};
}
}
}

#endif /* WORLD_PARALLEL_HASHLIFE_NODE_H_ */
