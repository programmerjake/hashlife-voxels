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
#include "file_stream.h"
#include <cstdio>
#include "../util/text.h"
#ifdef _WIN32
#include <wchar.h>
#endif

namespace programmerjake
{
namespace voxels
{
namespace io
{
struct FileInputStream::Implementation final
{
    std::FILE *file;
    explicit Implementation(FILE *file) : file(file)
    {
    }
    void close()
    {
        if(std::fclose(file) != 0)
        {
            file = nullptr;
            throw IOError(errno, std::generic_category(), "fclose failed");
        }
        file = nullptr;
    }
    ~Implementation()
    {
        std::fclose(file);
    }
};

FileInputStream::FileInputStream(std::string fileName)
{
#ifdef _WIN32
    typedef std::wstring FileNameType;
#else
    typedef std::string FileNameType;
#endif
    auto convertedFileName = util::text::stringCast<FileNameType>(std::move(fileName));
#ifdef _WIN32
    auto *file = ::_wfopen(convertedFileName.c_str(), "rbNS");
#elif defined(__linux)
    auto *file = std::fopen(convertedFileName.c_str(), "rbe");
#else
    auto *file = std::fopen(convertedFileName.c_str(), "rb");
#endif
    if(!file)
    {
        int error = errno;
        throw IOError(error, std::generic_category(), "fopen failed");
    }
    try
    {
        implementation = new Implementation(file);
    }
    catch(...)
    {
        std::fclose(file);
        throw;
    }
}

FileInputStream::~FileInputStream()
{
    delete implementation;
}

FileInputStream::ReadBytesResult FileInputStream::readBytes(
    unsigned char *buffer,
    std::size_t bufferSize,
    const std::chrono::steady_clock::time_point *timeout)
{
#error finish
}

void FileInputStream::close()
{
    if(implementation)
        implementation->close();
}
}
}
}
