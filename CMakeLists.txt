# Copyright (C) 2012-2017 Jacob R. Lifshay
# This file is part of Voxels.
#
# Voxels is free software; you can redistribute it and/or modify
# it under the terms of the GNU Lesser General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# Voxels is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public License
# along with Voxels; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
# MA 02110-1301, USA.
cmake_minimum_required(VERSION 3.1 FATAL_ERROR)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

project(voxels_hashlife CXX C)

find_package(SDL2 REQUIRED CONFIG)
include_directories(${SDL2_INCLUDE_DIRS})
# work around https://bugs.debian.org/cgi-bin/bugreport.cgi?bug=822210
string(STRIP ${SDL2_LIBRARIES} SDL2_LIBRARIES_STRIPPED)
link_libraries(${SDL2_LIBRARIES_STRIPPED})
add_compile_options(-Wall -ftemplate-depth=1024 -Werror "-Wno-error=cpp")
add_subdirectory(block)
add_subdirectory(graphics)
add_subdirectory(io)
add_subdirectory(lighting)
add_subdirectory(logging)
add_subdirectory(platform)
add_subdirectory(resource)
add_subdirectory(threading)
add_subdirectory(ui)
add_subdirectory(util)
add_subdirectory(world)

add_executable(voxels_hashlife
               main.cpp)
target_link_libraries(voxels_hashlife
                      block
                      graphics
                      io
                      lighting
                      logging
                      platform
                      resource
                      threading
                      ui
                      util
                      world)

