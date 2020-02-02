// Copyright (c) 2017 - 2017 Settlers Freaks (sf-team at siedler25.org)
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

#include "rttrDefines.h"
#include "mapGenerator/Algorithms.h"
#include "helpers/containerUtils.h"
#include <boost/test/unit_test.hpp>

using namespace rttr::mapGenerator;

BOOST_AUTO_TEST_SUITE(AlgorithmsTests)

BOOST_AUTO_TEST_CASE(Smooth_ForHomogenousNodes_KeepsNodesUnchanged)
{
    NodeMapBase<int> nodes;
    MapExtent size(16,8);

    const int radius = 4;
    const int iterations = 2;
    const int expectedValue = 1;
    
    nodes.Resize(size, expectedValue);

    Smooth(iterations, radius, nodes);
    
    RTTR_FOREACH_PT(MapPoint, size)
    {
        BOOST_REQUIRE(nodes[pt] == expectedValue);
    }
}

BOOST_AUTO_TEST_CASE(Smooth_ForSinglePeakValue_InterpolatesValuesAroundPeak)
{
    NodeMapBase<int> nodes;
    MapExtent size(16,8);
    
    const MapPoint peak(4,7);
    const int peakValue = 1000;
    const int radius = 4;
    const int iterations = 2;
    const int defaultValue = 1;
    
    nodes.Resize(size, defaultValue);
    nodes[peak] = peakValue;
    
    Smooth(iterations, radius, nodes);
    
    BOOST_REQUIRE(nodes[peak] < peakValue);
    
    auto neighbors = nodes.GetPointsInRadius(peak, radius);
    
    for (MapPoint p : neighbors)
    {
        BOOST_REQUIRE(nodes[p] > defaultValue);
    }
}

BOOST_AUTO_TEST_CASE(Scale_ForMixedValues_SetsMinAndMaxValuesCorrectly)
{
    ValueMap<unsigned> values;
    MapExtent size(16,8);
    
    values.Resize(size, 8); // default
    values[0] = 12; // max
    values[1] = 7; // min
    
    Scale(values, 0u, 20u);
    
    BOOST_REQUIRE(values[0] == 20u);
    BOOST_REQUIRE(values[1] == 0u);
}

BOOST_AUTO_TEST_CASE(Scale_ForEqualValues_DoesNotModifyAnyValues)
{
    ValueMap<unsigned> values;
    MapExtent size(16,8);
    
    values.Resize(size, 8);
    
    Scale(values, 0u, 20u);
    
    RTTR_FOREACH_PT(MapPoint, size)
    {
        BOOST_REQUIRE(values[pt] == 8);
    }
}

BOOST_AUTO_TEST_CASE(Collect_ForNegativeMapPoint_ReturnsEmptyVector)
{
    NodeMapBase<int> map;
    MapExtent size(16,8);
    MapPoint negative(0,0);
    
    map.Resize(size, 1);
    map[negative] = 0;
    
    auto result = Collect(map, negative, [&map](const MapPoint& p) {
        return map[p];
    });

    BOOST_REQUIRE(result.empty());
}

BOOST_AUTO_TEST_CASE(Collect_ForPositiveMap_ReturnsAllMapPoints)
{
    NodeMapBase<int> map;
    MapExtent size(16,8);
    MapPoint point(0,0);
    
    map.Resize(size, 1);
    
    auto result = Collect(map, point, [&map](const MapPoint& p) {
        return map[p];
    });

    BOOST_REQUIRE(result.size() == size.x * size.y);
    
    RTTR_FOREACH_PT(MapPoint, size)
    {
        BOOST_REQUIRE(helpers::contains(result, pt));
    }
}

BOOST_AUTO_TEST_CASE(Collect_ForMapWithPositiveAreas_ReturnsOnlyAreaAroundPoint)
{
    NodeMapBase<int> map;
    MapExtent size(16,16);
    map.Resize(size, 0);
    
    // positive area around target point
    MapPoint point(3,4);
    auto neighbors = map.GetNeighbours(point);
    for (auto neighbor : neighbors)
    {
        map[neighbor] = 1;
    }
    map[point] = 1;

    // disconnected, positive area
    MapPoint other(12,12);
    auto otherNeighbors = map.GetNeighbours(other);
    for (auto neighbor : otherNeighbors)
    {
        map[neighbor] = 1;
    }
    map[other] = 1;
    
    auto result = Collect(map, point, [&map](const MapPoint& p) {
        return map[p];
    });
    
    BOOST_REQUIRE(result.size() == neighbors.size() + 1);
    BOOST_REQUIRE(helpers::contains(result, point));

    for (auto pt : neighbors)
    {
        BOOST_REQUIRE(helpers::contains(result, pt));
    }
}

BOOST_AUTO_TEST_CASE(Distances_ForValuesAndEvaluator_ReturnsExpectedDistances)
{
    MapExtent size(8, 8);

    std::vector<int> expectedDistances {
        5, 5, 4, 3, 3, 3, 3, 4,
        5, 4, 3, 2, 2, 2, 3, 4,
        4, 4, 3, 2, 1, 1, 2, 3,
        4, 3, 2, 1, 0, 1, 2, 3,
        4, 4, 3, 2, 1, 1, 2, 3,
        5, 4, 3, 2, 2, 2, 3, 4,
        5, 5, 4, 3, 3, 3, 3, 4,
        6, 5, 4, 4, 4, 4, 4, 5
    };
    
    auto distances = Distances(size, [](MapPoint pt) {
        return pt.x == 4 && pt.y == 3; // compute distance to P(4/3)
    });
    
    BOOST_REQUIRE(distances.GetSize() == size);
    
    RTTR_FOREACH_PT(MapPoint, size)
    {
        BOOST_REQUIRE_EQUAL(expectedDistances[pt.x + pt.y * size.x], distances[pt]);
    }
}

BOOST_AUTO_TEST_CASE(Limit_ForMixedValuesAndCoverage_ReturnsValidLimit)
{
    ValueMap<int> values;
    MapExtent size(8,16);
    
    values.Resize(size);
    
    for (int i = 0; i < size.x * size.y; i++)
    {
        values[i] = i;
    }
    
    const float coverage = 0.3;
    const int minimum = 3;
    
    int limit = LimitFor(values, coverage, minimum);
    
    unsigned expectedNodes = static_cast<unsigned>(size.x * size.y * coverage);
    unsigned actualNodes = 0;
    
    RTTR_FOREACH_PT(MapPoint, size)
    {
        if (values[pt] >= minimum && values[pt] <= limit)
        {
            actualNodes++;
        }
    }

    BOOST_REQUIRE(actualNodes >= expectedNodes);
}

BOOST_AUTO_TEST_SUITE_END()
