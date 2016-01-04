// Copyright (c) 2005 - 2015 Settlers Freaks (sf-team at siedler25.org)
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

#ifndef InventorySetting_h__
#define InventorySetting_h__

/// Setting for each item in a warehouses inventory
/// These are bit flags!
enum InventorySetting{
    INV_SET_NONE = 0,
    INV_SET_STOP = 2,
    INV_SET_SEND = 4,
    INV_SET_STOP_AND_SEND = 6,
    INV_SET_COLLECT = 8
};

inline InventorySetting operator~(const InventorySetting a){ return static_cast<InventorySetting>(~static_cast<int>(a)); }

inline InventorySetting operator|(const InventorySetting a, const InventorySetting b){ return static_cast<InventorySetting>(static_cast<int>(a) | static_cast<int>(b)); }
inline InventorySetting operator&(const InventorySetting a, const InventorySetting b){ return static_cast<InventorySetting>(static_cast<int>(a) & static_cast<int>(b)); }
inline InventorySetting operator^(const InventorySetting a, const InventorySetting b){ return static_cast<InventorySetting>(static_cast<int>(a) ^ static_cast<int>(b)); }

inline InventorySetting& operator|=(InventorySetting& a, const InventorySetting b){ return a = a | b; }
inline InventorySetting& operator&=(InventorySetting& a, const InventorySetting b){ return a = a & b; }
inline InventorySetting& operator^=(InventorySetting& a, const InventorySetting b){ return a = a ^ b; }

#endif // InventorySetting_h__