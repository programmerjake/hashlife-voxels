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

#ifndef GRAPHICS_GRAPHICS_UTIL_TEXTURE_ATLAS_H_
#define GRAPHICS_GRAPHICS_UTIL_TEXTURE_ATLAS_H_

#include "../texture.h"
#include <type_traits>
#include <cstdint>
#include <vector>
#include <algorithm>
#include <utility>

namespace programmerjake
{
namespace voxels
{
namespace graphics
{
namespace graphics_util
{
namespace texture_atlas
{
struct TextureSize final
{
    std::size_t width, height;
    constexpr TextureSize(std::size_t width, std::size_t height) noexcept : width(width),
                                                                            height(height)
    {
    }
    constexpr TextureSize() noexcept : width(), height()
    {
    }
    explicit TextureSize(const Image &image) noexcept : width(image.width), height(image.height)
    {
    }
    constexpr bool operator==(const TextureSize &rt) const noexcept
    {
        return width == rt.width && height == rt.height;
    }
    constexpr bool operator!=(const TextureSize &rt) const noexcept
    {
        return !operator==(rt);
    }
    constexpr std::size_t getMaxDimension() const noexcept
    {
        return width > height ? width : height;
    }
    constexpr std::size_t getMinDimension() const noexcept
    {
        return width > height ? height : width;
    }

private:
    constexpr bool lessHelper(std::size_t maxDimension,
                              std::size_t minDimension,
                              const TextureSize &rt,
                              std::size_t rtMaxDimension,
                              std::size_t rtMinDimension) const noexcept
    {
        return maxDimension != rtMaxDimension ?
                   maxDimension < rtMaxDimension :
                   minDimension == rtMinDimension ? width < rt.width : minDimension
                                                                           < rtMinDimension;
    }

public:
    constexpr bool operator<(const TextureSize &rt) const noexcept
    {
        return lessHelper(
            getMaxDimension(), getMinDimension(), rt, rt.getMaxDimension(), rt.getMinDimension());
    }
    constexpr bool operator>(const TextureSize &rt) const noexcept
    {
        return rt.operator<(*this);
    }
    constexpr bool operator<=(const TextureSize &rt) const noexcept
    {
        return !(operator>(rt));
    }
    constexpr bool operator>=(const TextureSize &rt) const noexcept
    {
        return !(operator<(rt));
    }
};

template <typename T,
          std::size_t T::*XMember,
          std::size_t T::*YMember,
          const TextureSize T::*SizeMember>
struct TextureAtlas final
{
    typedef T TextureType;
    static constexpr std::size_t T::*xMember = XMember;
    static constexpr std::size_t T::*yMember = YMember;
    static constexpr const TextureSize T::*sizeMember = SizeMember;

private:
    struct TextureGroup final
    {
        TextureSize size;
        std::vector<T *> textures;
        TextureGroup(TextureSize size, std::vector<T *> textures)
            : size(size), textures(std::move(textures))
        {
        }
    };
    static std::uint64_t powerOf2Ceiling(std::uint64_t v) noexcept
    {
        v--;
        v |= v >> 1;
        v |= v >> 2;
        v |= v >> 4;
        v |= v >> 8;
        v |= v >> 16;
        v |= v >> 32;
        return v + 1;
    }
    static std::uint32_t powerOf2Ceiling(std::uint32_t v) noexcept
    {
        v--;
        v |= v >> 1;
        v |= v >> 2;
        v |= v >> 4;
        v |= v >> 8;
        v |= v >> 16;
        return v + 1;
    }
    static std::uint16_t powerOf2Ceiling(std::uint16_t v) noexcept
    {
        v--;
        v |= v >> 1;
        v |= v >> 2;
        v |= v >> 4;
        v |= v >> 8;
        return v + 1;
    }
    static bool textureGroupCompareFunction(const TextureGroup &a, const TextureGroup &b) noexcept
    {
        return a.size > b.size;
    }
    template <typename InputIterator>
    struct AddressOfIterator final
    {
        InputIterator iter;
        void operator++()
        {
            ++iter;
        }
        T *operator*()
        {
            return std::addressof(*iter);
        }
        template <typename Sentinel>
        bool operator==(Sentinel &&sentinel)
        {
            return iter == std::forward<Sentinel>(sentinel);
        }
        template <typename Sentinel>
        bool operator!=(Sentinel &&sentinel)
        {
            return iter != std::forward<Sentinel>(sentinel);
        }
        explicit AddressOfIterator(InputIterator iter) : iter(std::move(iter))
        {
        }
    };
    template <typename InputIterator, typename Sentinal>
    static TextureSize layoutImplementation(InputIterator inputBegin, Sentinal &&inputEnd)
    {
        std::vector<TextureGroup> textureGroups;
        for(InputIterator iter = std::move(inputBegin); iter != std::forward<Sentinal>(inputEnd);
            ++iter)
        {
            T *texture = *iter;
            texture->*xMember = 0;
            texture->*yMember = 0;
            TextureSize size = texture->*sizeMember;
            size.width = size.width == 0 ? 1 : powerOf2Ceiling(size.width);
            size.height = size.height == 0 ? 1 : powerOf2Ceiling(size.height);
            textureGroups.push_back(TextureGroup(size, {texture}));
            std::push_heap(textureGroups.begin(), textureGroups.end(), textureGroupCompareFunction);
        }
        if(textureGroups.empty())
            return TextureSize(1, 1);
        std::vector<TextureGroup> currentGroups;
        while(textureGroups.size() > 1)
        {
            currentGroups.clear();
            auto currentSize = textureGroups.front().size;
            do
            {
                currentGroups.push_back(TextureGroup(currentSize, {}));
                currentGroups.back().textures.swap(textureGroups.front().textures);
                std::pop_heap(
                    textureGroups.begin(), textureGroups.end(), textureGroupCompareFunction);
                textureGroups.pop_back();
            } while(!textureGroups.empty() && currentSize == textureGroups.front().size);
            auto newSize = currentSize;
            if(currentSize.width < currentSize.height)
                newSize.width *= 2;
            else
                newSize.height *= 2;
            while(!currentGroups.empty())
            {
                TextureGroup newGroup(newSize, {});
                newGroup.textures.swap(currentGroups.back().textures);
                currentGroups.pop_back();
                if(!currentGroups.empty())
                {
                    newGroup.textures.reserve(newGroup.textures.size()
                                              + currentGroups.back().textures.size());
                    if(currentSize.width < currentSize.height)
                    {
                        for(auto *texture : currentGroups.back().textures)
                        {
                            texture->*xMember += currentSize.width;
                            newGroup.textures.push_back(texture);
                        }
                    }
                    else
                    {
                        for(auto *texture : currentGroups.back().textures)
                        {
                            texture->*yMember += currentSize.height;
                            newGroup.textures.push_back(texture);
                        }
                    }
                    currentGroups.pop_back();
                }
                textureGroups.push_back(std::move(newGroup));
                std::push_heap(
                    textureGroups.begin(), textureGroups.end(), textureGroupCompareFunction);
            }
        }
        return textureGroups.back().size;
    }

public:
    template <
        typename InputIterator,
        typename Sentinal,
        typename =
            typename std::enable_if<std::is_convertible<decltype(*std::declval<InputIterator &>()),
                                                        T *>::value>::type>
    static TextureSize layout(InputIterator inputBegin, Sentinal &&inputEnd)
    {
        return layoutImplementation(std::move(inputBegin), std::forward<Sentinal>(inputEnd));
    }
    template <
        typename InputIterator,
        typename Sentinal,
        typename =
            typename std::enable_if<!std::is_convertible<decltype(*std::declval<InputIterator &>()),
                                                         T *>::value>::type,
        typename = void>
    static TextureSize layout(InputIterator inputBegin, Sentinal &&inputEnd)
    {
        return layoutImplementation(AddressOfIterator<InputIterator>(std::move(inputBegin)),
                                    std::forward<Sentinal>(inputEnd));
    }
};
}
}
}
}
}

#endif /* GRAPHICS_GRAPHICS_UTIL_TEXTURE_ATLAS_H_ */
