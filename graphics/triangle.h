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

#ifndef GRAPHICS_TRIANGLE_H_
#define GRAPHICS_TRIANGLE_H_

#include <type_traits>
#include "../util/vector.h"
#include "texture_coordinates.h"
#include "color.h"
#include "texture.h"
#include "transform.h"

namespace programmerjake
{
namespace voxels
{
namespace graphics
{
struct VertexWithoutNormal
{
    float positionX;
    float positionY;
    float positionZ;
    constexpr util::Vector3F getPosition() const
    {
        return util::Vector3F(positionX, positionY, positionZ);
    }
    TextureCoordinates::ValueType textureCoordinatesU;
    TextureCoordinates::ValueType textureCoordinatesV;
    constexpr TextureCoordinates getTextureCoordinates() const
    {
        return TextureCoordinates(textureCoordinatesU, textureCoordinatesV);
    }
    ColorI::ValueType colorRed;
    ColorI::ValueType colorGreen;
    ColorI::ValueType colorBlue;
    ColorI::ValueType colorOpacity;
    constexpr ColorI getColor() const
    {
        return ColorI(colorRed, colorGreen, colorBlue, colorOpacity);
    }
    constexpr VertexWithoutNormal()
        : positionX(),
          positionY(),
          positionZ(),
          textureCoordinatesU(),
          textureCoordinatesV(),
          colorRed(),
          colorGreen(),
          colorBlue(),
          colorOpacity()
    {
    }
    constexpr VertexWithoutNormal(float positionX,
                                  float positionY,
                                  float positionZ,
                                  TextureCoordinates::ValueType textureCoordinatesU,
                                  TextureCoordinates::ValueType textureCoordinatesV,
                                  ColorI::ValueType colorRed,
                                  ColorI::ValueType colorGreen,
                                  ColorI::ValueType colorBlue,
                                  ColorI::ValueType colorOpacity)
        : positionX(positionX),
          positionY(positionY),
          positionZ(positionZ),
          textureCoordinatesU(textureCoordinatesU),
          textureCoordinatesV(textureCoordinatesV),
          colorRed(colorRed),
          colorGreen(colorGreen),
          colorBlue(colorBlue),
          colorOpacity(colorOpacity)
    {
    }
    constexpr VertexWithoutNormal(util::Vector3F position,
                                  TextureCoordinates textureCoordinates,
                                  ColorI color)
        : positionX(position.x),
          positionY(position.y),
          positionZ(position.z),
          textureCoordinatesU(textureCoordinates.u),
          textureCoordinatesV(textureCoordinates.v),
          colorRed(color.red),
          colorGreen(color.green),
          colorBlue(color.blue),
          colorOpacity(color.opacity)
    {
    }
    friend constexpr VertexWithoutNormal transform(const Transform &tform,
                                                   const VertexWithoutNormal &vertex) noexcept
    {
        return VertexWithoutNormal(transform(tform, vertex.getPosition()),
                                   vertex.getTextureCoordinates(),
                                   vertex.getColor());
    }
};

struct Vertex : public VertexWithoutNormal
{
    float normalX;
    float normalY;
    float normalZ;
    constexpr util::Vector3F getNormal() const
    {
        return util::Vector3F(normalX, normalY, normalZ);
    }
    constexpr Vertex() : VertexWithoutNormal(), normalX(), normalY(), normalZ()
    {
    }
    constexpr Vertex(float positionX,
                     float positionY,
                     float positionZ,
                     TextureCoordinates::ValueType textureCoordinatesU,
                     TextureCoordinates::ValueType textureCoordinatesV,
                     ColorI::ValueType colorRed,
                     ColorI::ValueType colorGreen,
                     ColorI::ValueType colorBlue,
                     ColorI::ValueType colorOpacity,
                     float normalX,
                     float normalY,
                     float normalZ)
        : VertexWithoutNormal(positionX,
                              positionY,
                              positionZ,
                              textureCoordinatesU,
                              textureCoordinatesV,
                              colorRed,
                              colorGreen,
                              colorBlue,
                              colorOpacity),
          normalX(normalX),
          normalY(normalY),
          normalZ(normalZ)
    {
    }
    constexpr Vertex(util::Vector3F position,
                     TextureCoordinates textureCoordinates,
                     ColorI color,
                     util::Vector3F normal)
        : VertexWithoutNormal(position, textureCoordinates, color),
          normalX(normal.x),
          normalY(normal.y),
          normalZ(normal.z)
    {
    }
    friend constexpr Vertex transform(const Transform &tform, const Vertex &vertex) noexcept
    {
        return Vertex(transform(tform, vertex.getPosition()),
                      vertex.getTextureCoordinates(),
                      vertex.getColor(),
                      transformNormal(tform, vertex.getNormal()));
    }
};

constexpr util::Vector3F getTriangleNormalUnnormalized(util::Vector3F position1,
                                                       util::Vector3F position2,
                                                       util::Vector3F position3)
{
    return cross(position1 - position2, position1 - position3);
}

template <typename Metric = util::EuclideanMetric>
constexpr util::Vector3F getTriangleNormalOrZero(util::Vector3F position1,
                                                 util::Vector3F position2,
                                                 util::Vector3F position3,
                                                 Metric = util::euclideanMetric)
{
    return getTriangleNormalUnnormalized(position1, position2, position3).normalizeOrZero(Metric());
}

struct TriangleWithoutNormal
{
    static constexpr std::size_t vertexCount = 3;
    VertexWithoutNormal vertices[vertexCount];
    TextureId texture;
    constexpr TriangleWithoutNormal() : vertices(), texture()
    {
    }
    constexpr TriangleWithoutNormal(VertexWithoutNormal vertex1,
                                    VertexWithoutNormal vertex2,
                                    VertexWithoutNormal vertex3,
                                    TextureId texture)
        : vertices{vertex1, vertex2, vertex3}, texture(texture)
    {
    }
    friend constexpr TriangleWithoutNormal transform(const Transform &tform,
                                                     const TriangleWithoutNormal &triangle) noexcept
    {
        return TriangleWithoutNormal(transform(tform, triangle.vertices[0]),
                                     transform(tform, triangle.vertices[1]),
                                     transform(tform, triangle.vertices[2]),
                                     triangle.texture);
    }
};

struct Triangle
{
    static constexpr std::size_t vertexCount = 3;
    Vertex vertices[vertexCount];
    TextureId texture;
    constexpr Triangle() : vertices(), texture()
    {
    }
    constexpr Triangle(Vertex vertex1, Vertex vertex2, Vertex vertex3, TextureId texture)
        : vertices{vertex1, vertex2, vertex3}, texture(texture)
    {
    }
    friend constexpr Triangle transform(const Transform &tform, const Triangle &triangle) noexcept
    {
        return Triangle(transform(tform, triangle.vertices[0]),
                        transform(tform, triangle.vertices[1]),
                        transform(tform, triangle.vertices[2]),
                        triangle.texture);
    }
};
}
}
}

#endif /* GRAPHICS_TRIANGLE_H_ */
