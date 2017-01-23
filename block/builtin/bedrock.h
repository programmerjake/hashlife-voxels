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

#ifndef BLOCK_BUILTIN_BEDROCK_H_
#define BLOCK_BUILTIN_BEDROCK_H_

#include "stone.h"

namespace programmerjake
{
namespace voxels
{
namespace block
{
namespace builtin
{
class Bedrock final : public GenericStone
{
private:
    Bedrock();

public:
    static const Bedrock *get()
    {
        static const Bedrock *retval = new Bedrock;
        return retval;
    }
    static void init()
    {
        get();
    }
};
}
}
}
}

#endif /* BLOCK_BUILTIN_BEDROCK_H_ */
