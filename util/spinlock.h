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

#ifndef UTIL_SPINLOCK_H_
#define UTIL_SPINLOCK_H_

#include <mutex>
#include <atomic>

namespace programmerjake
{
namespace voxels
{
namespace util
{
class Spinlock final
{
    Spinlock(const Spinlock &) = delete;
    Spinlock &operator=(const Spinlock &) = delete;

private:
    std::atomic_flag flag = ATOMIC_FLAG_INIT;

public:
    constexpr Spinlock() noexcept
    {
    }
    void lock() noexcept
    {
        while(flag.test_and_set(std::memory_order_acquire))
        {
#if defined(__GNUC__) && (defined(__x86_64__) || defined(__i386__))
            __asm__("rep; nop"); // pause
#endif
        }
    }
    void unlock() noexcept
    {
        flag.clear(std::memory_order_release);
    }
    bool try_lock() noexcept
    {
        return !flag.test_and_set(std::memory_order_acquire);
    }
};
}
}
}

#endif /* UTIL_SPINLOCK_H_ */
