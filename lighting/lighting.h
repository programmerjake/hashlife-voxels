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
#ifndef LIGHTING_LIGHTING_H_
#define LIGHTING_LIGHTING_H_

#include "../world/dimension.h"
#include "../util/constexpr_assert.h"
#include "../graphics/color.h"
#include "../util/vector.h"
#include "../util/interpolate.h"

namespace programmerjake
{
namespace voxels
{
namespace lighting
{
struct Lighting final
{
    typedef world::Dimension::Properties::LightValueType LightValueType;
    static constexpr int lightBitWidth = world::Dimension::Properties::lightBitWidth;
    static constexpr LightValueType maxLight =
        (static_cast<LightValueType>(1) << lightBitWidth) - 1;
    static_assert(maxLight == world::Dimension::Properties::maxLight, "");
    static constexpr float toFloat(LightValueType v)
    {
        return static_cast<float>(static_cast<int>(v)) / static_cast<int>(maxLight);
    }
    static constexpr LightValueType ensureInValidRange(LightValueType v)
    {
        return constexprAssert(v >= 0 && v <= maxLight), v;
    }
    struct GlobalProperties final
    {
        LightValueType skylight;
        world::Dimension dimension;
        constexpr GlobalProperties() : skylight(maxLight)
        {
        }
        constexpr explicit GlobalProperties(LightValueType skylight, world::Dimension dimension)
            : skylight(skylight), dimension(dimension)
        {
        }
        constexpr bool operator==(const GlobalProperties &rt) const
        {
            return skylight == rt.skylight && dimension == rt.dimension;
        }
        constexpr bool operator!=(const GlobalProperties &rt) const
        {
            return !operator==(rt);
        }
        std::size_t hash() const
        {
            return std::hash<LightValueType>()(skylight)
                   + 9 * std::hash<world::Dimension>()(dimension);
        }
    };

public:
    LightValueType directSkylight;
    LightValueType indirectSkylight;
    LightValueType indirectArtificalLight;

private:
    template <typename T>
    static constexpr T constexpr_max(T a, T b)
    {
        return (a < b) ? b : a;
    }
    template <typename T>
    static constexpr T constexpr_min(T a, T b)
    {
        return (a < b) ? a : b;
    }

public:
    constexpr LightValueType getBrightnessLevel(LightValueType skylight) const
    {
        return constexpr_max<int>(static_cast<int>(indirectSkylight) - maxLight + skylight,
                                  indirectArtificalLight);
    }
    constexpr LightValueType getBrightnessLevel(const GlobalProperties &globalProperties) const
    {
        return getBrightnessLevel(globalProperties.skylight);
    }
    constexpr float toFloat(LightValueType skylight, float zeroBrightnessLevel) const
    {
        return toFloat(getBrightnessLevel(skylight)) * (1 - zeroBrightnessLevel)
               + zeroBrightnessLevel;
    }
    float toFloat(const GlobalProperties &globalProperties) const noexcept
    {
        return toFloat(globalProperties.skylight,
                       globalProperties.dimension.getZeroBrightnessLevel());
    }
    constexpr Lighting() : directSkylight(0), indirectSkylight(0), indirectArtificalLight(0)
    {
    }
    constexpr Lighting(LightValueType directSkylight,
                       LightValueType indirectSkylight,
                       LightValueType indirectArtificalLight)
        : directSkylight(ensureInValidRange(directSkylight)),
          indirectSkylight(ensureInValidRange(constexpr_max(directSkylight, indirectSkylight))),
          indirectArtificalLight(ensureInValidRange(indirectArtificalLight))
    {
    }
    enum MakeDirectOnlyType
    {
        makeDirectOnly
    };
    constexpr Lighting(LightValueType directSkylight,
                       LightValueType indirectSkylight,
                       LightValueType indirectArtificalLight,
                       MakeDirectOnlyType)
        : directSkylight(ensureInValidRange(directSkylight)),
          indirectSkylight(ensureInValidRange(indirectSkylight)),
          indirectArtificalLight(ensureInValidRange(indirectArtificalLight))
    {
    }
    static constexpr Lighting makeSkyLighting()
    {
        return Lighting(maxLight, maxLight, 0);
    }
    static constexpr Lighting makeDirectOnlyLighting()
    {
        return Lighting(maxLight, 1, 1, makeDirectOnly);
    }
    static constexpr Lighting makeArtificialLighting(LightValueType indirectArtificalLight)
    {
        return Lighting(0, 0, indirectArtificalLight);
    }
    static constexpr Lighting makeMaxLight()
    {
        return Lighting(maxLight, maxLight, maxLight);
    }
    constexpr Lighting combine(Lighting r) const
    {
        return Lighting(constexpr_max(directSkylight, r.directSkylight),
                        constexpr_max(indirectSkylight, r.indirectSkylight),
                        constexpr_max(indirectArtificalLight, r.indirectArtificalLight),
                        makeDirectOnly);
    }
    constexpr Lighting minimize(Lighting r) const
    {
        return Lighting(constexpr_min(directSkylight, r.directSkylight),
                        constexpr_min(indirectSkylight, r.indirectSkylight),
                        constexpr_min(indirectArtificalLight, r.indirectArtificalLight),
                        makeDirectOnly);
    }

private:
    static constexpr LightValueType sum(LightValueType a, LightValueType b)
    {
        return constexpr_min<LightValueType>(a + b, maxLight);
    }

public:
    constexpr Lighting sum(Lighting r) const
    {
        return Lighting(sum(directSkylight, r.directSkylight),
                        sum(indirectSkylight, r.indirectSkylight),
                        sum(indirectArtificalLight, r.indirectArtificalLight),
                        makeDirectOnly);
    }

private:
    static constexpr LightValueType reduce(LightValueType a, LightValueType b)
    {
        return (a < b ? 0 : a - b);
    }

public:
    constexpr Lighting reduce(Lighting r) const
    {
        return Lighting(reduce(directSkylight, r.directSkylight),
                        reduce(indirectSkylight, r.indirectSkylight),
                        reduce(indirectArtificalLight, r.indirectArtificalLight));
    }
    constexpr Lighting stripDirectSkylight() const
    {
        return Lighting(0, indirectSkylight, indirectArtificalLight);
    }
    constexpr bool operator==(Lighting r) const
    {
        return directSkylight == r.directSkylight && indirectSkylight == r.indirectSkylight
               && indirectArtificalLight == r.indirectArtificalLight;
    }
    constexpr bool operator!=(Lighting r) const
    {
        return !operator==(r);
    }
};

struct LightProperties final
{
    Lighting emissiveValue;
    Lighting reduceValue;
    constexpr LightProperties(Lighting emissiveValue, Lighting reduceValue)
        : emissiveValue(emissiveValue), reduceValue(reduceValue)
    {
    }
    static constexpr LightProperties transparent(Lighting emissiveValue = Lighting(0, 0, 0))
    {
        return LightProperties(emissiveValue, Lighting(0, 1, 1));
    }
    static constexpr LightProperties blocksDirectLight(Lighting emissiveValue = Lighting(0, 0, 0))
    {
        return LightProperties(emissiveValue, Lighting::makeDirectOnlyLighting());
    }
    static constexpr LightProperties opaque(Lighting emissiveValue = Lighting(0, 0, 0))
    {
        return LightProperties(emissiveValue, Lighting::makeMaxLight());
    }
    static constexpr LightProperties water(Lighting emissiveValue = Lighting(0, 0, 0))
    {
        return LightProperties(emissiveValue, Lighting(2, 3, 3));
    }
    constexpr Lighting eval(
        Lighting nx, Lighting px, Lighting ny, Lighting py, Lighting nz, Lighting pz) const
    {
        return emissiveValue.combine(nx.stripDirectSkylight().reduce(reduceValue))
            .combine(px.stripDirectSkylight().reduce(reduceValue))
            .combine(ny.stripDirectSkylight().reduce(reduceValue))
            .combine(py.reduce(reduceValue))
            .combine(nz.stripDirectSkylight().reduce(reduceValue))
            .combine(pz.stripDirectSkylight().reduce(reduceValue));
    }
    constexpr Lighting eval(Lighting inputLighting) const
    {
        return emissiveValue.combine(inputLighting.reduce(reduceValue));
    }
    constexpr Lighting createNewLighting(Lighting oldLighting) const
    {
        return emissiveValue.combine(
            oldLighting.minimize(Lighting::makeMaxLight().reduce(reduceValue)));
    }
    constexpr Lighting calculateTransmittedLighting(Lighting lighting) const
    {
        return lighting.reduce(reduceValue);
    }
    template <typename... Args>
    constexpr Lighting calculateTransmittedLighting(Lighting lighting, Args... args) const
    {
        return calculateTransmittedLighting(lighting)
            .combine(calculateTransmittedLighting(args...));
    }
    constexpr LightProperties compose(LightProperties inFrontValue) const
    {
        return LightProperties(inFrontValue.emissiveValue.combine(
                                   inFrontValue.calculateTransmittedLighting(emissiveValue)),
                               reduceValue.sum(inFrontValue.reduceValue));
    }
    constexpr LightProperties combine(LightProperties other) const
    {
        return LightProperties(other.emissiveValue.combine(emissiveValue),
                               other.reduceValue.minimize(reduceValue));
    }
    constexpr bool isTotallyTransparent() const
    {
        return reduceValue.indirectSkylight <= 1 && reduceValue.indirectArtificalLight <= 1;
    }
};

struct BlockLighting final
{
    std::array<std::array<std::array<float, 2>, 2>, 2> lightValues;
    constexpr graphics::ColorF eval(util::Vector3F relativePosition) const
    {
        using util::interpolate;
        return graphics::grayscaleF(interpolate(
            relativePosition.x,
            interpolate(
                relativePosition.y,
                interpolate(relativePosition.z, lightValues[0][0][0], lightValues[0][0][1]),
                interpolate(relativePosition.z, lightValues[0][1][0], lightValues[0][1][1])),
            interpolate(
                relativePosition.y,
                interpolate(relativePosition.z, lightValues[1][0][0], lightValues[1][0][1]),
                interpolate(relativePosition.z, lightValues[1][1][0], lightValues[1][1][1]))));
    }

private:
    float evalVertex(const std::array<std::array<std::array<float, 3>, 3>, 3> &blockValues,
                     util::Vector3I32 offset);

public:
    constexpr BlockLighting() : lightValues{{{{{{0, 0}}, {{0, 0}}}}, {{{{0, 0}}, {{0, 0}}}}}}
    {
    }
    BlockLighting(
        std::array<std::array<std::array<std::pair<LightProperties, Lighting>, 3>, 3>, 3> blocks,
        const Lighting::GlobalProperties &globalProperties);

private:
    static constexpr util::Vector3F getLightVector()
    {
        return util::Vector3F(0, 1, 0);
    }
    static constexpr float getNormalFactorHelper(float v)
    {
        return 0.5f + (v < 0 ? v * 0.25f : v * 0.5f);
    }
    static constexpr float getNormalFactor(util::Vector3F normal)
    {
        return 0.4f + 0.6f * getNormalFactorHelper(dot(normal, getLightVector()));
    }

public:
    constexpr graphics::ColorF lightVertex(util::Vector3F relativePosition,
                                           graphics::ColorF vertexColor,
                                           util::Vector3F normal) const
    {
        return colorize(
            colorize(graphics::grayscaleAF(getNormalFactor(normal), 1), eval(relativePosition)),
            vertexColor);
    }
};
}
}
}

namespace std
{
template <>
struct hash<programmerjake::voxels::lighting::Lighting::GlobalProperties>
{
    size_t operator()(
        const programmerjake::voxels::lighting::Lighting::GlobalProperties &globalProperties) const
    {
        return globalProperties.hash();
    }
};
}

#endif // LIGHTING_LIGHTING_H_
