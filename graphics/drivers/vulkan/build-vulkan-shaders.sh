#!/bin/bash
#
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

if [[ -x "$GLSLC" ]]; then
    :
elif [[ "" != "$GLSLC" ]]; then
    echo "invalid path for glslc: $GLSLC" >&2
    exit 1
elif GLSLC="`which glslc`"; then
    :
else
    echo "can't find glslc" >&2
    exit 1
fi
echo "found glslc: $GLSLC"

function compile()
{
    local fileName="$1" variableName="$2"
    glslc -c -mfmt=bin -o "${fileName}.spv" "${fileName}" || return 1
    cat > "${fileName}.h" <<EOF
// Automatically generated from ${fileName}
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

const std::uint32_t ${variableName}[] =
{
EOF
    hexdump -v -e '/4 "    0x%08X,\n"' < "${fileName}.spv" >> "${fileName}.h"
    cat >> "${fileName}.h" <<EOF
};
EOF
}

compile vulkan.frag vulkanFragmentShader
compile vulkan.vert vulkanVertexShader


