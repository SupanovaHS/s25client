// Copyright (c) 2005 - 2017 Settlers Freaks (sf-team at siedler25.org)
//
// This file is part of Return To The Roots.
//
// Return To The Roots is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 2 of the License, or
// (at your option) any later version.
//
// Return To The Roots is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with Return To The Roots. If not, see <http://www.gnu.org/licenses/>.

#ifndef Triangles_h__
#define Triangles_h__

#include "world/MapGeometry.h"
#include <vector>

namespace rttr {
namespace mapGenerator {

struct Triangle
{
    const bool rsu;
    const MapPoint position;
    
    Triangle(bool rsu, const MapPoint& position);

    /**
     * Creates a new, inverted triangle at the same position.
     * @returns a new, inverted triangle (RSU -> LSD, LSD -> RSU).
     */
    Triangle Inverse() const;
    
    /**
     * Creates a new, inverted triangle with the specified position.
     * @param position position of the triangle
     * @returns a new, inverted triangle (RSU -> LSD, LSD -> RSU).
     */
    Triangle Inverse(const MapPoint& position) const;
};

/**
 * Finds all triangles connected to the specified map point.
 * @param p center point of the triangles
 * @param size map size
 * @returns a list of all triangles which are connected to the point.
 */
std::vector<Triangle> GetTriangles(const MapPoint& p, const MapExtent& size);

/**
 * Computes all neighboring triangles for the specified triangle.
 * @param triangle triangle to find neighbors for
 * @param size map size
 * @returns all three neighboring triangles
 */
std::vector<Triangle> GetTriangleNeighbors(const Triangle& triangle, const MapExtent& size);

/**
 * Computes the edge points for the specified triangle - useful for interpolation.
 * @param triangle triangle to find edge points for
 * @param size size of the map
 * @returns edge points of the triangle.
 */
std::vector<MapPoint> GetTriangleEdges(const Triangle& triangle, const MapExtent& size);

}}

#endif // Triangles_h__
