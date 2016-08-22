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
