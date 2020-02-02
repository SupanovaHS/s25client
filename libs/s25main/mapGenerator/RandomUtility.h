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

#ifndef RandomUtility_h__
#define RandomUtility_h__

#include "gameTypes/MapCoordinates.h"
#include "random/XorShift.h"
#include <vector>

namespace rttr {
namespace mapGenerator {

using UsedRNG = XorShift;

class RandomUtility
{
private:
    UsedRNG rng_;
    
public:
    RandomUtility();
    RandomUtility(uint64_t seed);
    
    /**
     * Returns 'true' by a %-chance.
     * @param percentage likelihood to return true
     */
    bool ByChance(int percentage);
    
    /**
     * Generates a random index based on the specified size.
     * @param size range of indices (0 to size - 1)
     * @return a random index based on the specified size.
     */
    int Index(const size_t& size);
    
    /**
     * Returns a random item.
     * @param items collection of all items to choose from
     * @returns a random item out of the specified collection of items.
     */
    template<typename T>
    T RandomItem(std::vector<T> items)
    {
        return items[Index(items.size())];
    }
    
    /**
     * Generates a random position for a map of the specified size.
     * @param size boundaries of the map
     * @return a random position on the map.
     */
    Position RandomPoint(const MapExtent& size);
    
    /**
     * Generates a random integer number between the specified minimum and maximum values.
     * @param min minimum value for the random number
     * @param max maximum value for the random number
     * @return a random number between min and max (inclusive).
     */
    int Rand(int min, int max);
    
    /**
     * Generates a random floating point value between the specified minimum and maximum values.
     * @param min minimum value for the random number
     * @param max maximum value for the random number
     * @return a random float value between min and max (inclusive).
     */
    double DRand(double min, double max);
    
    /**
     * Generates "n" unique random numbers within the range of "0" to "n-1".
     * @param n number and range of indices to generate
     * @return a vector of "n" random number within the range of "0" to "n-1".
     */
    std::vector<int> ShuffledRange(int n);
    
    /**
     * Shuffles the specified vector of positions.
     * @param area positions to shuffle.
     */
    void Shuffle(std::vector<Position>& area);
};

}}

#endif // RandomUtility_h__

