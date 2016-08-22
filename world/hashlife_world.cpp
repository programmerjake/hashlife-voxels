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
#include "hashlife_world.h"
#include <ostream>
#include <unordered_map>
#include <deque>

namespace programmerjake
{
namespace voxels
{
namespace world
{
void HashlifeWorld::dumpNode(HashlifeNodeBase *node, std::ostream &os)
{
    std::unordered_map<HashlifeNodeBase *, std::size_t> nodeNumbers;
    std::deque<HashlifeNodeBase *> worklist;
    worklist.push_back(node);
    nodeNumbers[node] = 0;
    while(!worklist.empty())
    {
        auto currentNode = worklist.front();
        worklist.pop_front();
        os << "#" << nodeNumbers.at(currentNode) << ": (" << static_cast<const void *>(currentNode)
           << ")\n    level = " << static_cast<unsigned>(currentNode->level) << "\n";
        if(currentNode->isLeaf())
        {
            for(util::Vector3I32 position(0); position.x < HashlifeNodeBase::levelSize;
                position.x++)
            {
                for(position.y = 0; position.y < HashlifeNodeBase::levelSize; position.y++)
                {
                    for(position.z = 0; position.z < HashlifeNodeBase::levelSize; position.z++)
                    {
                        auto block =
                            static_cast<HashlifeLeafNode *>(currentNode)->getBlock(position);
                        auto blockDescriptor = block::BlockDescriptor::get(block.getBlockKind());
                        os << "    [" << position.x << "][" << position.y << "][" << position.z
                           << "] = <" << static_cast<unsigned>(block.getDirectSkylight()) << ", "
                           << static_cast<unsigned>(block.getIndirectSkylight()) << ", "
                           << static_cast<unsigned>(block.getIndirectArtificalLight()) << "> ";
                        if(blockDescriptor)
                            os << blockDescriptor->name;
                        else
                            os << "<empty>";
                        os << "\n";
                    }
                }
            }
        }
        else
        {
            for(util::Vector3I32 position(0); position.x < HashlifeNodeBase::levelSize;
                position.x++)
            {
                for(position.y = 0; position.y < HashlifeNodeBase::levelSize; position.y++)
                {
                    for(position.z = 0; position.z < HashlifeNodeBase::levelSize; position.z++)
                    {
                        auto childNode =
                            static_cast<HashlifeNonleafNode *>(currentNode)->getChildNode(position);
                        auto iter = nodeNumbers.find(childNode);
                        if(iter == nodeNumbers.end())
                        {
                            iter = std::get<0>(nodeNumbers.emplace(childNode, nodeNumbers.size()));
                            worklist.push_back(childNode);
                        }
                        os << "    [" << position.x << "][" << position.y << "][" << position.z
                           << "] = #" << std::get<1>(*iter) << "\n";
                    }
                }
            }
        }
        os << "\n";
    }
    os.flush();
}
}
}
}
