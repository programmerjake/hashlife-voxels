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

#ifndef LOGGING_LOGGING_H_
#define LOGGING_LOGGING_H_

#include <string>
#include <type_traits>
#include <atomic>
#include <cassert>

namespace programmerjake
{
namespace voxels
{
namespace logging
{
enum class Level
{
    Debug = 0,
    Info = 1,
    Warning = 2,
    Error = 3,
    Fatal = 4
};

inline const char *getLevelNameString(Level level) noexcept
{
    switch(level)
    {
    case Level::Debug:
        return "Debug";
    case Level::Info:
        return "Info";
    case Level::Warning:
        return "Warning";
    case Level::Error:
        return "Error";
    case Level::Fatal:
        return "Fatal";
    }
    assert(false);
    return "";
}

struct Logger final
{
private:
    static std::atomic<Level> globalLevel;
    static void doLog(Level level,
                      const char *subsystem,
                      const char *message,
                      std::size_t messageLength) noexcept;

public:
    static Level getGlobalLevel() noexcept
    {
        return globalLevel.load(std::memory_order_relaxed);
    }
    static void setGlobalLevel(Level level) noexcept
    {
        globalLevel.store(level, std::memory_order_relaxed);
    }
    static void log(Level level,
                    const char *subsystem,
                    const char *message,
                    std::size_t messageLength) noexcept
    {
        if(level >= getGlobalLevel())
            doLog(level, subsystem, message, messageLength);
    }
};

inline Level getGlobalLevel() noexcept
{
    return Logger::getGlobalLevel();
}

inline void setGlobalLevel(Level level) noexcept
{
    Logger::setGlobalLevel(level);
}

inline void log(Level level,
                const char *subsystem,
                const char *message,
                std::size_t messageLength) noexcept
{
    Logger::log(level, subsystem, message, messageLength);
}

inline void log(Level level, const char *subsystem, const char *message) noexcept
{
    Logger::log(level, subsystem, message, std::char_traits<char>::length(message));
}

inline void log(Level level, const char *subsystem, const std::string &message) noexcept
{
    Logger::log(level, subsystem, message.data(), message.size());
}
}
}
}

#endif /* LOGGING_LOGGING_H_ */
