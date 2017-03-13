#!/bin/bash
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
#
#

if false; then
exec > >(tee simd_generated-formatted.h | clang-format-3.6 > simd_generated.h)
else
exec > simd_generated.h
fi

echo "/* automatically generated file; generated by generate_simd.sh */
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
#include \"simd.h\"
#ifndef UTIL_SIMD_GENERATED_H_
#define UTIL_SIMD_GENERATED_H_

namespace programmerjake
{
namespace voxels
{
namespace util
{"

seperator=""
types=(
    "std::uint8_t     1 u8Value"
    "std::int8_t      1 i8Value"
    "std::uint16_t    2 u16Value"
    "std::int16_t     2 i16Value"
    "std::uint32_t    4 u32Value"
    "std::int32_t     4 i32Value"
    "SIMD256::float32 4 f32Value"
    "std::uint64_t    8 u64Value"
    "std::int64_t     8 i64Value"
    "SIMD256::float64 8 f64Value")

for fromTypePair in "${types[@]}"; do
    fromTypeArray=($fromTypePair)
    fromType="${fromTypeArray[0]}"
    fromTypeSize="${fromTypeArray[1]}"
    fromVariable="${fromTypeArray[2]}"
    for toTypePair in "${types[@]}"; do
        toTypeArray=($toTypePair)
        toType="${toTypeArray[0]}"
        toTypeSize="${toTypeArray[1]}"
        toVariable="${toTypeArray[2]}"
        echo -n "${seperator}"
        echo -n "template <typename S>
struct SIMD256::ConvertHelper<$toType, $fromType, S> final
{
    static constexpr "
        sourceVectorCount=$((fromTypeSize))
        destVectorCount=$((toTypeSize))
        while ((sourceVectorCount > 1 && destVectorCount > 1)); do
            ((sourceVectorCount /= 2))
            ((destVectorCount /= 2))
        done
        if ((destVectorCount > 1)); then
            echo -n "std::array<SIMD256, $destVectorCount>"
        else
            echo -n "SIMD256"
        fi
        echo -n " run("
        if ((sourceVectorCount > 1)); then
            echo -n "const SIMD256 &v0"
            if ((sourceVectorCount > 2)); then
                for((i=1;i<sourceVectorCount;i++)); do
                    echo -n ",
                                 const SIMD256 &v$i"
                done
            else
                for((i=1;i<sourceVectorCount;i++)); do
                    echo -n ", const SIMD256 &v$i"
                done
            fi
        else
            echo -n "const SIMD256 &v"
        fi
        echo -n ") noexcept
    {
        return "
        indentFromMultipleOutputs=""
        if ((destVectorCount > 1)); then
            echo -n "{"
            indentFromMultipleOutputs=" "
        fi
        for((element = 0; element < sourceVectorCount * 32 / fromTypeSize; element++)); do
            if ((element % (32 / toTypeSize) == 0)); then
                if ((element)); then
                    echo -n ",
               $indentFromMultipleOutputs"
                fi
                echo -n "SIMD256("
            else
                echo -n ",
                       $indentFromMultipleOutputs"
            fi
            echo -n "static_cast<$toType>(v"
            if ((sourceVectorCount > 1)); then
                echo -n "$((element / (32 / fromTypeSize)))"
            fi
            echo -n ".$fromVariable[$((element % (32 / fromTypeSize)))])"
            if (((element + 1) % (32 / toTypeSize) == 0)); then
                echo -n ")"
            fi
        done
        if ((destVectorCount > 1)); then
            echo -n "}"
        fi
        echo ";
    }
};"
        seperator=$'\n'
    done
done

echo "}
}
}

#endif /* UTIL_SIMD_GENERATED_H_ */"

