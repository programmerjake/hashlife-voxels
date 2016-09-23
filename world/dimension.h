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

#ifndef WORLD_DIMENSION_H_
#define WORLD_DIMENSION_H_

#include <cstdint>
#include <functional>
#include <string>
#include <vector>
#include <cstdint>
#include <tuple>
#include <initializer_list>
#include "../util/optional.h"
#include "../util/integer_sequence.h"
#include <stdexcept>

namespace programmerjake
{
namespace voxels
{
namespace world
{
struct Dimension final
{
    struct Properties final
    {
        typedef std::uint_fast8_t LightValueType;
        static constexpr int lightBitWidth = 4;
        static constexpr LightValueType maxLight =
            (static_cast<LightValueType>(1) << lightBitWidth) - 1;
        float zeroBrightnessLevel;
        std::string name;
        bool hasDayNightCycle;
        LightValueType maxSkylightLevel;
        explicit Properties(float zeroBrightnessLevel,
                            std::string name,
                            bool hasDayNightCycle,
                            LightValueType maxSkylightLevel)
            : zeroBrightnessLevel(zeroBrightnessLevel),
              name(std::move(name)),
              hasDayNightCycle(hasDayNightCycle),
              maxSkylightLevel(maxSkylightLevel)
        {
        }
    };

private:
    static std::vector<Properties> *makePropertiesLookupTable() noexcept;
    static void handleTooManyDimensions() noexcept;
    static std::vector<Properties> &getPropertiesLookupTable() noexcept
    {
        static std::vector<Properties> *retval = makePropertiesLookupTable();
        return *retval;
    }

public:
    typedef std::uint8_t ValueType;
    ValueType value = 0;
    constexpr Dimension() = default;
    constexpr explicit Dimension(ValueType value) : value(value)
    {
    }
    static constexpr Dimension overworld()
    {
        return Dimension();
    }
    static constexpr Dimension nether()
    {
        return Dimension(overworld().value + 1);
    }
    static constexpr Dimension lastPredefinedDimension()
    {
        return nether();
    }
    static Dimension allocate(Properties properties) noexcept
    {
        auto &propertiesLookupTable = getPropertiesLookupTable();
        Dimension retval(propertiesLookupTable.size());
        if(retval.value != propertiesLookupTable.size())
            handleTooManyDimensions();
        propertiesLookupTable.push_back(std::move(properties));
        return retval;
    }
    static void init()
    {
        getPropertiesLookupTable();
    }
    const Properties &getProperties() const noexcept
    {
        return getPropertiesLookupTable()[value];
    }
    float getZeroBrightnessLevel() const noexcept
    {
        return getProperties().zeroBrightnessLevel;
    }
    const std::string &getName() const noexcept
    {
        return getProperties().name;
    }
    constexpr bool operator==(const Dimension &rt) const
    {
        return value == rt.value;
    }
    constexpr bool operator!=(const Dimension &rt) const
    {
        return value != rt.value;
    }
};

template <typename T>
class DimensionMap final
{
private:
    typedef std::pair<const Dimension, T> ValueType2;

public:
    typedef Dimension key_type;
    typedef T mapped_type;
    typedef ValueType2 value_type;

private:
    std::vector<util::Optional<value_type>> elements;
    std::size_t fullElementCount = 0;

private:
    util::Optional<value_type> *getElement(Dimension dimension) noexcept
    {
        std::size_t index = dimension.value;
        if(index >= elements.size())
            return nullptr;
        return &elements[index];
    }
    const util::Optional<value_type> *getElement(Dimension dimension) const noexcept
    {
        std::size_t index = dimension.value;
        if(index >= elements.size())
            return nullptr;
        return &elements[index];
    }
    util::Optional<value_type> &getOrMakeElement(Dimension dimension)
    {
        std::size_t index = dimension.value;
        if(index <= elements.size())
            elements.resize(index + 1);
        return elements[index];
    }

public:
    T &operator[](Dimension dimension)
    {
        auto &element = getOrMakeElement(dimension);
        if(!element)
        {
            element.emplace(
                std::piecewise_construct, std::make_tuple(dimension), std::make_tuple());
            fullElementCount++;
        }
        return std::get<1>(*element);
    }
    T &at(Dimension dimension)
    {
        auto *element = getElement(dimension);
        if(!element || !*element)
            throw std::out_of_range("DimensionMap::at");
        return std::get<1>(**element);
    }
    const T &at(Dimension dimension) const
    {
        auto *element = getElement(dimension);
        if(!element || !*element)
            throw std::out_of_range("DimensionMap::at");
        return std::get<1>(**element);
    }
    std::size_t count(Dimension dimension) const noexcept
    {
        auto *element = getElement(dimension);
        if(!element || !*element)
            return 0;
        return 1;
    }
    std::size_t size() const noexcept
    {
        return fullElementCount;
    }
    bool empty() const noexcept
    {
        return fullElementCount == 0;
    }
    class iterator;
    class const_iterator final
    {
        friend class DimensionMap;
        friend class iterator;

    public:
        typedef std::forward_iterator_tag iterator_category;
        typedef ValueType2 value_type;
        typedef std::ptrdiff_t difference_type;
        typedef const value_type *pointer;
        typedef const value_type &reference;

    private:
        std::size_t index;
        const std::vector<util::Optional<value_type>> *elements;
        constexpr const_iterator(std::size_t indexIn,
                                 const std::vector<util::Optional<value_type>> *elements) noexcept
            : index(indexIn),
              elements(elements)
        {
        }
        const_iterator &moveToValidPosition() noexcept
        {
            while(index < elements->size() && !(*elements)[index])
                index++;
            return *this;
        }

    public:
        constexpr const_iterator() noexcept : index(0), elements(nullptr)
        {
        }
        const value_type &operator*() const noexcept
        {
            return *(*elements)[index];
        }
        const value_type *operator->() const noexcept
        {
            return &operator*();
        }
        const_iterator &operator++() noexcept
        {
            index++;
            while(index < elements->size() && !(*elements)[index])
                index++;
            return *this;
        }
        const_iterator operator++(int) noexcept
        {
            auto retval = *this;
            operator++();
            return retval;
        }
        friend constexpr bool operator==(const const_iterator &a, const const_iterator &b) noexcept
        {
            return a.index == b.index;
        }
        friend constexpr bool operator!=(const const_iterator &a, const const_iterator &b) noexcept
        {
            return a.index != b.index;
        }
    };
    class iterator final
    {
        friend class DimensionMap;

    public:
        typedef std::forward_iterator_tag iterator_category;
        typedef ValueType2 value_type;
        typedef std::ptrdiff_t difference_type;
        typedef value_type *pointer;
        typedef value_type &reference;

    private:
        std::size_t index;
        std::vector<util::Optional<value_type>> *elements;
        constexpr iterator(std::size_t indexIn,
                           std::vector<util::Optional<value_type>> *elements) noexcept
            : index(indexIn),
              elements(elements)
        {
        }
        iterator &moveToValidPosition() noexcept
        {
            index = operator const_iterator().moveToValidPosition().index;
            return *this;
        }

    public:
        constexpr iterator() noexcept : index(0), elements(nullptr)
        {
        }
        value_type &operator*() const noexcept
        {
            return *(*elements)[index];
        }
        value_type *operator->() const noexcept
        {
            return &operator*();
        }
        iterator &operator++() noexcept
        {
            index++;
            while(index < elements->size() && !(*elements)[index])
                index++;
            return *this;
        }
        iterator operator++(int) noexcept
        {
            auto retval = *this;
            operator++();
            return retval;
        }
        friend constexpr bool operator==(const iterator &a, const iterator &b) noexcept
        {
            return a.index == b.index;
        }
        friend constexpr bool operator!=(const iterator &a, const iterator &b) noexcept
        {
            return a.index != b.index;
        }
        constexpr operator const_iterator() const noexcept
        {
            return const_iterator(index, elements);
        }
    };
    iterator begin() noexcept
    {
        return iterator(0, &elements).moveToValidPosition();
    }
    iterator end() noexcept
    {
        return iterator(elements.size(), &elements);
    }
    const_iterator begin() const noexcept
    {
        return const_iterator(0, &elements).moveToValidPosition();
    }
    const_iterator end() const noexcept
    {
        return const_iterator(elements.size(), &elements);
    }
    const_iterator cbegin() const noexcept
    {
        return const_iterator(0, &elements).moveToValidPosition();
    }
    const_iterator cend() const noexcept
    {
        return const_iterator(elements.size(), &elements);
    }
    void clear() noexcept
    {
        for(auto &element : elements)
            element = util::nullOpt;
        fullElementCount = 0;
    }
    void swap(DimensionMap &rt) noexcept
    {
        std::swap(*this, rt);
    }
    iterator find(Dimension dimension) noexcept
    {
        auto *element = getElement(dimension);
        if(!element || !*element)
            return end();
        return iterator(dimension.value, &elements);
    }
    const_iterator find(Dimension dimension) const noexcept
    {
        auto *element = getElement(dimension);
        if(!element || !*element)
            return end();
        return iterator(dimension.value, &elements);
    }
    std::pair<iterator, iterator> equal_range(Dimension dimension) noexcept
    {
        auto retval = find(dimension);
        auto next = retval;
        if(retval != end())
            ++next;
        return std::pair<iterator, iterator>(retval, next);
    }
    std::pair<const_iterator, const_iterator> equal_range(Dimension dimension) const noexcept
    {
        auto retval = find(dimension);
        auto next = retval;
        if(retval != end())
            ++next;
        return std::pair<const_iterator, const_iterator>(retval, next);
    }

private:
    template <typename... Args, std::size_t... indices>
    static Dimension getKeyHelper(std::tuple<Args...> &&args,
                                  util::IntegerSequence<std::size_t, indices...>)
    {
        return Dimension(std::get<indices>(std::move(args))...);
    }
    template <typename... Args>
    static Dimension getKey(std::tuple<Args...> &&args)
    {
        return getKeyHelper(std::move(args), util::IndexSequenceFor<Args...>());
    }
    template <typename ValueArgsTuple>
    std::pair<iterator, bool> emplaceHelper2(Dimension key, ValueArgsTuple &&valueArgsTuple)
    {
        auto &element = getOrMakeElement(key);
        if(element)
        {
            value_type(std::piecewise_construct,
                       std::make_tuple(key),
                       std::forward<ValueArgsTuple>(valueArgsTuple)); // call constructor anyways
            return std::pair<iterator, bool>(iterator(key.value, &elements), false);
        }
        element.emplace(std::piecewise_construct,
                        std::make_tuple(key),
                        std::forward<ValueArgsTuple>(valueArgsTuple));
        return std::pair<iterator, bool>(iterator(key.value, &elements), true);
    }
    template <typename... KeyArgs, typename... ValueArgs>
    std::pair<iterator, bool> emplaceHelper(std::piecewise_construct_t,
                                            std::tuple<KeyArgs...> keyArgs,
                                            const std::tuple<ValueArgs...> &valueArgs)
    {
        return emplaceHelper2(getKey(std::move(keyArgs)), valueArgs);
    }
    template <typename... KeyArgs, typename... ValueArgs>
    std::pair<iterator, bool> emplaceHelper(std::piecewise_construct_t,
                                            std::tuple<KeyArgs...> keyArgs,
                                            std::tuple<ValueArgs...> &&valueArgs)
    {
        return emplaceHelper2(getKey(std::move(keyArgs)), std::move(valueArgs));
    }
    std::pair<iterator, bool> emplaceHelper()
    {
        return emplaceHelper2(Dimension(), std::make_tuple());
    }
    template <typename Key, typename Value>
    std::pair<iterator, bool> emplaceHelper(Key &&key, Value &&value)
    {
        return emplaceHelper2(Dimension(std::forward<Key>(key)),
                              std::forward_as_tuple(std::forward<Value>(value)));
    }
    template <typename Key, typename Value>
    std::pair<iterator, bool> emplaceHelper(const std::pair<Key, Value> &entry)
    {
        return emplaceHelper2(Dimension(std::get<0>(entry)),
                              std::forward_as_tuple(std::get<1>(entry)));
    }
    template <typename Key, typename Value>
    std::pair<iterator, bool> emplaceHelper(std::pair<Key, Value> &&entry)
    {
        return emplaceHelper2(Dimension(std::move(std::get<0>(entry))),
                              std::forward_as_tuple(std::move(std::get<1>(entry))));
    }

public:
    template <typename... Args>
    std::pair<iterator, bool> emplace(Args &&... args)
    {
        return emplaceHelper(std::forward<Args>(args)...);
    }
    template <typename... Args>
    iterator emplace_hint(const_iterator hint, Args &&... args)
    {
        return std::get<0>(emplaceHelper(std::forward<Args>(args)...));
    }
    std::pair<iterator, bool> insert(const value_type &entry)
    {
        return emplaceHelper(entry);
    }
    std::pair<iterator, bool> insert(value_type &&entry)
    {
        return emplaceHelper(std::move(entry));
    }
    template <typename U, typename = decltype(value_type(std::declval<U>()))>
    std::pair<iterator, bool> insert(U &&entry)
    {
        return emplaceHelper(std::forward<U>(entry));
    }
    iterator insert(const_iterator hint, const value_type &entry)
    {
        return std::get<0>(emplaceHelper(entry));
    }
    iterator insert(const_iterator hint, value_type &&entry)
    {
        return std::get<0>(emplaceHelper(std::move(entry)));
    }
    template <typename U, typename = decltype(value_type(std::declval<U>()))>
    std::pair<iterator, bool> insert(const_iterator hint, U &&entry)
    {
        return emplaceHelper(std::forward<U>(entry));
    }
    template <typename InputIterator>
    void insert(InputIterator first, InputIterator last)
    {
        while(first != last)
        {
            insert(*first);
            ++first;
        }
    }
    void insert(std::initializer_list<value_type> initializerList)
    {
        for(auto &entry : initializerList)
        {
            insert(entry);
        }
    }
    std::size_t erase(Dimension dimension)
    {
        auto *element = getElement(dimension);
        if(element && *element)
        {
            element->reset();
            return 1;
        }
        return 0;
    }
    iterator erase(const_iterator position)
    {
        iterator retval(position.index, &elements);
        ++retval;
        elements[position.index].reset();
        return retval;
    }
    iterator erase(const_iterator first, const_iterator last)
    {
        while(first != last)
        {
            first = erase(first);
        }
        return first;
    }
};
}
}
}

namespace std
{
template <>
struct hash<programmerjake::voxels::world::Dimension>
{
    std::size_t operator()(programmerjake::voxels::world::Dimension v) const
    {
        return std::hash<programmerjake::voxels::world::Dimension::ValueType>()(v.value);
    }
};
}

#endif /* WORLD_DIMENSION_H_ */
