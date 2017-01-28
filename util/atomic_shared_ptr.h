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

#ifndef UTIL_ATOMIC_SHARED_PTR_H_
#define UTIL_ATOMIC_SHARED_PTR_H_

#include <memory>
#include <atomic>

namespace programmerjake
{
namespace voxels
{
namespace util
{
template <typename T>
class atomic_shared_ptr final
{
    atomic_shared_ptr(const atomic_shared_ptr &) = delete;
    atomic_shared_ptr &operator=(const atomic_shared_ptr &) = delete;

private:
    std::shared_ptr<T> value;

public:
    constexpr atomic_shared_ptr() noexcept : value()
    {
    }
    atomic_shared_ptr(std::shared_ptr<T> value) noexcept : value(std::move(value))
    {
    }
    void operator=(std::shared_ptr<T> newValue) noexcept
    {
        store(std::move(newValue));
    }
    bool is_lock_free() const noexcept
    {
        return std::atomic_is_lock_free(&value);
    }
    void store(std::shared_ptr<T> newValue,
               std::memory_order order = std::memory_order_seq_cst) noexcept
    {
        std::atomic_store_explicit(&value, std::move(newValue), order);
    }
    std::shared_ptr<T> load(std::memory_order order = std::memory_order_seq_cst) const noexcept
    {
        return std::atomic_load_explicit(&value, order);
    }
    operator std::shared_ptr<T>() const noexcept
    {
        return load();
    }
    std::shared_ptr<T> exchange(std::shared_ptr<T> newValue,
                                std::memory_order order = std::memory_order_seq_cst) noexcept
    {
        return std::atomic_exchange_explicit(&value, std::move(newValue), order);
    }
    // compare_exchange_* are not implemented
};
}
}
}

#endif /* UTIL_ATOMIC_SHARED_PTR_H_ */
