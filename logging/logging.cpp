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
#include "logging.h"
#include <iostream>
#include <mutex>

namespace programmerjake
{
namespace voxels
{
namespace logging
{
std::atomic<Level> Logger::globalLevel(Level::Warning);

namespace
{
std::mutex loggerLock;
}

void Logger::doLog(Level level,
                   const char *subsystem,
                   const char *message,
                   std::size_t messageLength) noexcept
{
    std::unique_lock<std::mutex> lockIt(loggerLock);
    std::cout << getLevelNameString(level) << ":" << subsystem << ": ";
    std::cout.write(message, messageLength);
    std::cout << std::endl;
}
}
}
}
