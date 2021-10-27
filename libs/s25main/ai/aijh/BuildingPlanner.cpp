// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "BuildingPlanner.h"
#include "AIPlayerJH.h"
#include "GlobalGameSettings.h"
#include "addons/const_addons.h"
#include "buildings/nobBaseWarehouse.h"
#include "buildings/nobMilitary.h"
#include "buildings/noBuildingSite.h"

#include "gameTypes/BuildingType.h"
#include "gameTypes/GoodTypes.h"

#include "gameData/BuildingConsts.h"
#include "gameData/BuildingProperties.h"
#include <algorithm>
#include <cmath>

namespace AIJH {
BuildingPlanner::BuildingPlanner(const AIPlayerJH& aijh) : buildingsWanted(), expansionRequired(false)
{
    RefreshBuildingNums(aijh);
    InitBuildingsWanted(aijh);
    UpdateBuildingsWanted(aijh);
}

void BuildingPlanner::Update(unsigned gf, AIPlayerJH& aijh)
{
    RefreshBuildingNums(aijh);
    expansionRequired = CalcIsExpansionRequired(aijh, gf > 500 && gf % 500 == 0);
}

void BuildingPlanner::RefreshBuildingNums(const AIPlayerJH& aijh)
{
    buildingNums = aijh.player.GetBuildingRegister().GetBuildingNums();
}

bool BuildingPlanner::CalcIsExpansionRequired(AIPlayerJH& aijh, bool recalc) const
{
    if(!expansionRequired && !recalc)
        return false;
    if(GetNumMilitaryBldSites() > 0 || GetNumMilitaryBlds() > 4)
        return false;
    bool hasWood = GetNumBuildings(BuildingType::Woodcutter) > 0;
    bool hasBoards = GetNumBuildings(BuildingType::Sawmill) > 0;
    bool hasStone = GetNumBuildings(BuildingType::Quarry) > 0;
    if(expansionRequired)
    {
        if(hasWood && hasBoards && hasStone)
            return false;
    } else
    {
        // Check if we could build the missing building around any military building or storehouse. If not, expand.
        const BuildingRegister& buildingRegister = aijh.player.GetBuildingRegister();
        std::vector<noBuilding*> blds(buildingRegister.GetMilitaryBuildings().begin(),
                                      buildingRegister.GetMilitaryBuildings().end());
        blds.insert(blds.end(), buildingRegister.GetStorehouses().begin(), buildingRegister.GetStorehouses().end());
        for(const noBuilding* bld : blds)
        {
            if(!hasWood)
                hasWood = aijh.FindPositionForBuildingAround(BuildingType::Woodcutter, bld->GetPos()).isValid();
            if(!hasBoards)
                hasBoards = aijh.FindPositionForBuildingAround(BuildingType::Sawmill, bld->GetPos()).isValid();
            if(!hasStone)
                hasStone = aijh.FindPositionForBuildingAround(BuildingType::Quarry, bld->GetPos()).isValid();
        }
        return !(hasWood && hasBoards && hasStone);
    }
    return expansionRequired;
}

unsigned BuildingPlanner::GetNumBuildings(BuildingType type) const
{
    return buildingNums.buildings[type] + buildingNums.buildingSites[type];
}

unsigned BuildingPlanner::GetNumBuildingSites(BuildingType type) const
{
    return buildingNums.buildingSites[type];
}

unsigned BuildingPlanner::GetNumMilitaryBlds() const
{
    unsigned result = 0;
    for(BuildingType bld : BuildingProperties::militaryBldTypes)
        result += GetNumBuildings(bld);
    return result;
}

unsigned BuildingPlanner::GetNumMilitaryBldSites() const
{
    unsigned result = 0;
    for(BuildingType bld : BuildingProperties::militaryBldTypes)
        result += GetNumBuildingSites(bld);
    return result;
}

void BuildingPlanner::InitBuildingsWanted(const AIPlayerJH& aijh)
{
    std::fill(buildingsWanted.begin(), buildingsWanted.end(), 0u);
  /*  buildingsWanted[BuildingType::Forester] = 1;
    buildingsWanted[BuildingType::Sawmill] = 1;
    buildingsWanted[BuildingType::Woodcutter] = 12;
    buildingsWanted[BuildingType::GraniteMine] = 0;
    buildingsWanted[BuildingType::CoalMine] = 4;
    buildingsWanted[BuildingType::IronMine] = 2;
    buildingsWanted[BuildingType::GoldMine] = 1;
    buildingsWanted[BuildingType::Catapult] = 5;
    buildingsWanted[BuildingType::Fishery] = 6;
    buildingsWanted[BuildingType::Quarry] = 6;
    buildingsWanted[BuildingType::Hunter] = 2;
    buildingsWanted[BuildingType::Farm] =
      aijh.player.GetInventory().goods[GoodType::Scythe] + aijh.player.GetInventory().people[Job::Farmer];*/

    unsigned numAIRelevantSeaIds = aijh.GetNumAIRelevantSeaIds();
    if(numAIRelevantSeaIds > 0)
    {
        buildingsWanted[BuildingType::HarborBuilding] = 99;
        buildingsWanted[BuildingType::Shipyard] = numAIRelevantSeaIds == 1 ? 1 : 99;
    }
}

void BuildingPlanner::UpdateBuildingsWanted(const AIPlayerJH& aijh)
{
    // lets get a solid start on building materials.........

    // no military buildings -> usually start only

    const Inventory& inventory = aijh.player.GetInventory();

    // get soldier count
    unsigned numSoldiers = 0;
    for(auto i : SOLDIER_JOBS)
        numSoldiers += inventory.people[i];

    // use either our occupied military buildings count or soldier count to scale, whichever is the lowest
    unsigned occupiedMilitary = 0;
    for(const auto& mil : aijh.player.GetBuildingRegister().GetMilitaryBuildings())
    {
        occupiedMilitary += (mil->GetNumTroops()) ? 1 : 0; // only increase if occupied, don't count all occupants
    }

    const unsigned& militaryPower = std::min(occupiedMilitary, numSoldiers);

   

    // we can also check building sites, and if all resources are either waiting to be built OR en-route, consider that
    // as complete to move on to next stage
    const auto& buildingSites = aijh.player.GetBuildingRegister().GetBuildingSites();
    std::array<BuildingType, 8> bldSites = {{BuildingType::Sawmill, BuildingType::GraniteMine, BuildingType::IronMine,
                                             BuildingType::CoalMine, BuildingType::Ironsmelter,
                                             BuildingType::Metalworks, BuildingType::Farm, BuildingType::Forester}};
    helpers::EnumArray<unsigned, BuildingType> bldCount{};
    for(const auto site : buildingSites)
    {
        const auto type = site->GetBuildingType();
        if(!helpers::contains(bldSites, type)) 
            continue;
        // get resources used, waiting and en-route
        const auto& totalBoards = site->getBoards() + site->getUsedBoards() + site->getOrderedBoards().size();
        const auto& totalStone = site->getStones() + site->getUsedStones() + site->getOrderedStones().size();

        if(totalBoards == BUILDING_COSTS[type].boards && totalStone == BUILDING_COSTS[type].stones)      
            ++bldCount[type];
    }

    
    const auto& foresterCount = aijh.player.GetBuildingRegister().GetBuildings(BuildingType::Forester).size() + bldCount[BuildingType::Forester];
    const auto& sawmillCount =
      aijh.player.GetBuildingRegister().GetBuildings(BuildingType::Sawmill).size() + bldCount[BuildingType::Sawmill];
    // mines
    const auto& graniteCount = aijh.player.GetBuildingRegister().GetBuildings(BuildingType::GraniteMine).size()
                               + bldCount[BuildingType::GraniteMine];
    const auto& ironCount =
      aijh.player.GetBuildingRegister().GetBuildings(BuildingType::IronMine).size() + bldCount[BuildingType::IronMine];
    const auto& coalCount =
      aijh.player.GetBuildingRegister().GetBuildings(BuildingType::CoalMine).size() + bldCount[BuildingType::CoalMine];

    const auto& ironsmelterCount = aijh.player.GetBuildingRegister().GetBuildings(BuildingType::Ironsmelter).size()
                                  + bldCount[BuildingType::Ironsmelter];
    const auto& metalworksCount = aijh.player.GetBuildingRegister().GetBuildings(BuildingType::Metalworks).size()
                                   + bldCount[BuildingType::Metalworks];

    const auto& farmCount =
     aijh.player.GetBuildingRegister().GetBuildings(BuildingType::Farm).size() + bldCount[BuildingType::Farm];
    

    /*const auto calcFarmsWanted = [](const unsigned pigFarms, const unsigned mills, const unsigned donkeyFarms,
                                    const unsigned charBurners, const unsigned brewery) {
        return pigFarms + mills  + donkeyFarms
               + (charBurners * 2) + brewery;
    };
    */

    if(sawmillCount == 0) // new game (potentially, or we've been F****D over)
    {
        buildingsWanted[BuildingType::Sawmill] = 1u;
        buildingsWanted[BuildingType::Forester] = 1u;
        buildingsWanted[BuildingType::Woodcutter] = 2u;
        buildingsWanted[BuildingType::Quarry] = 0u;
        buildingsWanted[BuildingType::GraniteMine] = 0u;
        buildingsWanted[BuildingType::CoalMine] = 0u;
        buildingsWanted[BuildingType::IronMine] = 0u;
        buildingsWanted[BuildingType::GoldMine] = 0u;
        buildingsWanted[BuildingType::Catapult] = 0u;
        buildingsWanted[BuildingType::Fishery] = 0u;
        buildingsWanted[BuildingType::Hunter] = 0u;
        buildingsWanted[BuildingType::Farm] = 0u;
        buildingsWanted[BuildingType::Charburner] = 0u;
        buildingsWanted[BuildingType::Ironsmelter] = 0u;
        buildingsWanted[BuildingType::Metalworks] = 0u;
        return;
    }

    if(metalworksCount < 1 || ironsmelterCount < 1)
    {
        buildingsWanted[BuildingType::Ironsmelter] = 1u;
        buildingsWanted[BuildingType::Metalworks] = 1u;
        buildingsWanted[BuildingType::Quarry] = 1u;
        buildingsWanted[BuildingType::Fishery] = 1u;
        buildingsWanted[BuildingType::Hunter] = 1u;
        return;
    }


     if(sawmillCount < 2)
    {
        buildingsWanted[BuildingType::Sawmill] = 2u;
        buildingsWanted[BuildingType::Forester] =
          std::max(buildingsWanted[BuildingType::Sawmill], GetNumBuildings(BuildingType::Sawmill));
        buildingsWanted[BuildingType::Woodcutter] = buildingsWanted[BuildingType::Forester] * 2u;
        buildingsWanted[BuildingType::Barracks] = 20u;
        return;
    }

    if(coalCount < 1 || ironCount < 1)
    {
        buildingsWanted[BuildingType::CoalMine] = 1u;
        buildingsWanted[BuildingType::IronMine] = 1u;
        return;
    }

    if(militaryPower < 4)
        return;
    
    if(sawmillCount < 3)
    {
        buildingsWanted[BuildingType::Sawmill] = 3u;
        buildingsWanted[BuildingType::Forester] =
          std::max(buildingsWanted[BuildingType::Sawmill], GetNumBuildings(BuildingType::Sawmill)); 
        buildingsWanted[BuildingType::Woodcutter] = buildingsWanted[BuildingType::Forester] * 2u;
        buildingsWanted[BuildingType::Quarry] = 2u;
        buildingsWanted[BuildingType::Fishery] = 2u;
        buildingsWanted[BuildingType::Hunter] = 2u;
        buildingsWanted[BuildingType::GraniteMine] = 1u;
        buildingsWanted[BuildingType::GoldMine] = 1u;
        buildingsWanted[BuildingType::Mint] = 1u;
        return;
    }
  
    if(militaryPower < 8)
        return;
    if(coalCount < 3 || ironCount < 2
       || ironsmelterCount < 2  || metalworksCount < 2 )
    {
        buildingsWanted[BuildingType::Fishery] = 3u;
        buildingsWanted[BuildingType::Hunter] = 3u;
        buildingsWanted[BuildingType::CoalMine] = 3u;
        buildingsWanted[BuildingType::IronMine] = 2u;
        buildingsWanted[BuildingType::Ironsmelter] = 2u;
        buildingsWanted[BuildingType::Metalworks] = 2u;
        buildingsWanted[BuildingType::Armory] = 2u;
        buildingsWanted[BuildingType::Brewery] = 1u;
        buildingsWanted[BuildingType::GoldMine] = 1u;
        buildingsWanted[BuildingType::Mint] = 1u;
        buildingsWanted[BuildingType::Well] = 1u;
        buildingsWanted[BuildingType::Farm] = 1u;
        return;
    }

    if(sawmillCount < 5)
    {
        buildingsWanted[BuildingType::Sawmill] = 5u;
        buildingsWanted[BuildingType::Forester] =
          std::max(buildingsWanted[BuildingType::Sawmill], GetNumBuildings(BuildingType::Sawmill)); 
        buildingsWanted[BuildingType::Woodcutter] = buildingsWanted[BuildingType::Forester] * 3u;
        buildingsWanted[BuildingType::Quarry] = 3u;
        buildingsWanted[BuildingType::Fishery] = 4u;
        buildingsWanted[BuildingType::Hunter] = 4u;
        buildingsWanted[BuildingType::GraniteMine] = 1u;
        return;
    }


    // Unleash the beast
    buildingsWanted[BuildingType::Quarry] = militaryPower / 4u;
    buildingsWanted[BuildingType::GraniteMine] = 2 + militaryPower / 40u;

    buildingsWanted[BuildingType::Catapult] =
      militaryPower <= 5 || inventory.goods[GoodType::Stones] < 50 ?
        0 :
        std::min((inventory.goods[GoodType::Stones] - 50) / 4, GetNumBuildings(BuildingType::Catapult) + 4);

    if(aijh.ggs.GetMaxMilitaryRank() != 0) // max rank is 0 = private / recruit ==> gold is useless!
    {
        buildingsWanted[BuildingType::GoldMine] = militaryPower / 16u;
        buildingsWanted[BuildingType::Mint] = buildingsWanted[BuildingType::GoldMine];
    }

    buildingsWanted[BuildingType::Ironsmelter] = militaryPower / 12 + 1u;
    buildingsWanted[BuildingType::Armory] = militaryPower / 15 + 1u;

    buildingsWanted[BuildingType::Brewery] =
      (buildingsWanted[BuildingType::Armory] / 4) < 1 ? (buildingsWanted[BuildingType::Armory] / 4) : 1;

    buildingsWanted[BuildingType::Metalworks] = militaryPower / 30u;

    buildingsWanted[BuildingType::CoalMine] = buildingsWanted[BuildingType::Ironsmelter]
                                              + buildingsWanted[BuildingType::Armory]
                                              + buildingsWanted[BuildingType::Mint] + 3u;

    buildingsWanted[BuildingType::IronMine] = 2 + buildingsWanted[BuildingType::Ironsmelter];

    buildingsWanted[BuildingType::Mill] = militaryPower / 12u;
    buildingsWanted[BuildingType::Bakery] = militaryPower / 12u;
    buildingsWanted[BuildingType::PigFarm] = militaryPower / 14u;
    buildingsWanted[BuildingType::Slaughterhouse] = 1u + militaryPower / 14u;
    buildingsWanted[BuildingType::DonkeyBreeder] = 1u;
    buildingsWanted[BuildingType::Fishery] = militaryPower / 4u;
    buildingsWanted[BuildingType::Hunter] = militaryPower / 4u;

    buildingsWanted[BuildingType::Well] = buildingsWanted[BuildingType::Bakery] + buildingsWanted[BuildingType::PigFarm]
                                          + buildingsWanted[BuildingType::DonkeyBreeder]
                                          + buildingsWanted[BuildingType::Brewery];

    buildingsWanted[BuildingType::Charburner] =
      buildingsWanted[BuildingType::CoalMine] - coalCount; // fill any gaps with char burners
    buildingsWanted[BuildingType::Woodcutter] += buildingsWanted[BuildingType::Charburner];

    buildingsWanted[BuildingType::Farm] = buildingsWanted[BuildingType::PigFarm] + buildingsWanted[BuildingType::Mill]
                                          + buildingsWanted[BuildingType::DonkeyBreeder]
                                          + (buildingsWanted[BuildingType::Charburner] * 2u)
                                          + buildingsWanted[BuildingType::Brewery] + 3u;

     buildingsWanted[BuildingType::Sawmill] = militaryPower / 6u;
    buildingsWanted[BuildingType::Forester] =
      (GetNumBuildings(BuildingType::Charburner) / 3u)
       + std::max(buildingsWanted[BuildingType::Sawmill], GetNumBuildings(BuildingType::Sawmill)); 
    
     buildingsWanted[BuildingType::Woodcutter] = buildingsWanted[BuildingType::Forester] * 3u;


    return;

    // older "script" no longer used below


    // at least some expansion happened -> more buildings wanted
    // building wanted usually limited by profession workers+tool for profession with some arbitrary limit. Some
    // buildings which are linked to others in a chain / profession-tool-rivalry have additional limits.

    // check our wood and stone supplies, if its bad, clear all wanted and only order wood/stone buildings
    const int bldsites = aijh.player.GetBuildingRegister().GetBuildingSites().size();

    // throttle wanted demand...

    if(bldsites > static_cast<int>(militaryPower))
        return; // do nothing?

    if(bldsites > static_cast<int>(inventory[GoodType::Stones]) * 2
       || bldsites > static_cast<int>((inventory[GoodType::Boards] + inventory[GoodType::Wood])) * 2)
    {
        buildingsWanted[BuildingType::Quarry] = militaryPower / 4;
        if(buildingsWanted[BuildingType::Quarry] < 8)
            buildingsWanted[BuildingType::Quarry] = 8;
        if(buildingsWanted[BuildingType::Quarry] > 16)
            buildingsWanted[BuildingType::Quarry] = 16;

        buildingsWanted[BuildingType::GraniteMine] = buildingsWanted[BuildingType::Quarry] / 2;

        buildingsWanted[BuildingType::Woodcutter] = buildingsWanted[BuildingType::Quarry];

        buildingsWanted[BuildingType::CoalMine] = 0;
        buildingsWanted[BuildingType::IronMine] = 0;
        buildingsWanted[BuildingType::GoldMine] = 0;
        buildingsWanted[BuildingType::Catapult] = 0;
        buildingsWanted[BuildingType::Fishery] = 0;
        buildingsWanted[BuildingType::Hunter] = 0;
        buildingsWanted[BuildingType::Farm] = 0;
        buildingsWanted[BuildingType::Charburner] = 0;
        buildingsWanted[BuildingType::Well] = 0;

        return;
    }

    // Wood

    buildingsWanted[BuildingType::Forester] = ((militaryPower / 10u) + 1u) < 20u ? (militaryPower / 10u) + 1u : 20u;
    buildingsWanted[BuildingType::Woodcutter] = buildingsWanted[BuildingType::Forester] * 2 + 1;

    // sawmills limited by woodcutters and carpenter+saws reduced by char burners minimum of 3
    int resourcelimit = inventory.people[Job::Carpenter] + inventory.goods[GoodType::Saw] + 2;
    int numSawmillsFed = (static_cast<int>(GetNumBuildings(BuildingType::Woodcutter))
                          - static_cast<int>(GetNumBuildings(BuildingType::Charburner) * 2))
                         / 2;
    buildingsWanted[BuildingType::Sawmill] = std::max(std::min(numSawmillsFed, resourcelimit), 3); // min 3

    // Stone
    buildingsWanted[BuildingType::Quarry] = ((militaryPower / 10u) + 1u) < 20u ? (militaryPower / 10u) + 1u : 20u;
    buildingsWanted[BuildingType::GraniteMine] = ((militaryPower / 20u) + 1u) < 7u ? (militaryPower / 20u) + 1u : 7u;

    // fishery & hunter
    buildingsWanted[BuildingType::Fishery] =
      std::min(inventory.goods[GoodType::RodAndLine] + inventory.people[Job::Fisher], militaryPower / 10u + 1u);
    buildingsWanted[BuildingType::Hunter] =
      std::min(inventory.goods[GoodType::Bow] + inventory.people[Job::Hunter], militaryPower / 10u + 1u);

    // iron smelters limited by iron mines or crucibles
    buildingsWanted[BuildingType::Ironsmelter] =
      std::min(inventory.goods[GoodType::Crucible] + inventory.people[Job::IronFounder] + 2,
               GetNumBuildings(BuildingType::IronMine));

    // mints check gold mines
    buildingsWanted[BuildingType::Mint] = GetNumBuildings(BuildingType::GoldMine);

    // armory count = smelter -metalworks if there is more than 1 smelter or 1 if there is just 1.
    buildingsWanted[BuildingType::Armory] =
      (GetNumBuildings(BuildingType::Ironsmelter) > 1) ?
        std::max(0, static_cast<int>(GetNumBuildings(BuildingType::Ironsmelter))
                      - static_cast<int>(GetNumBuildings(BuildingType::Metalworks))) :
        GetNumBuildings(BuildingType::Ironsmelter);
    if(aijh.ggs.isEnabled(AddonId::HALF_COST_MIL_EQUIP))
        buildingsWanted[BuildingType::Armory] *= 2;
    // brewery count = 1+(armory/5) if there is at least 1 armory or armory /6 for exhaustible mines
    if(GetNumBuildings(BuildingType::Armory) > 0 && GetNumBuildings(BuildingType::Farm) > 0)
    {
        if(aijh.ggs.isEnabled(AddonId::INEXHAUSTIBLE_MINES))
            buildingsWanted[BuildingType::Brewery] = 1 + GetNumBuildings(BuildingType::Armory) / 5;
        else
            buildingsWanted[BuildingType::Brewery] = 1 + GetNumBuildings(BuildingType::Armory) / 6;
    } else
        buildingsWanted[BuildingType::Brewery] = 0;

    // metalworks
    while(buildingsWanted[BuildingType::Metalworks] != GetNumBuildings(BuildingType::Ironsmelter) / 2)
        buildingsWanted[BuildingType::Metalworks] = GetNumBuildings(BuildingType::Ironsmelter) / 2;

    // max processing
    unsigned foodusers = GetNumBuildings(BuildingType::Charburner) + GetNumBuildings(BuildingType::Mill)
                         + GetNumBuildings(BuildingType::Brewery) + GetNumBuildings(BuildingType::PigFarm)
                         + GetNumBuildings(BuildingType::DonkeyBreeder);

    if(GetNumBuildings(BuildingType::Farm) >= foodusers - GetNumBuildings(BuildingType::Mill))
        buildingsWanted[BuildingType::Mill] =
          std::min(GetNumBuildings(BuildingType::Farm) - (foodusers - GetNumBuildings(BuildingType::Mill)),
                   GetNumBuildings(BuildingType::Bakery) + 1);
    else
        buildingsWanted[BuildingType::Mill] = GetNumBuildings(BuildingType::Mill);

    resourcelimit = inventory.people[Job::Baker] + inventory.goods[GoodType::Rollingpin] + 1;
    buildingsWanted[BuildingType::Bakery] = std::min<unsigned>(GetNumBuildings(BuildingType::Mill), resourcelimit);

    /*buildingsWanted[BuildingType::PigFarm] = (GetNumBuildings(BuildingType::Farm) < 8) ?
                                               GetNumBuildings(BuildingType::Farm) / 4 :
                                               (GetNumBuildings(BuildingType::Farm) - 2) / 4;
    if(buildingsWanted[BuildingType::PigFarm] > GetNumBuildings(BuildingType::Slaughterhouse) + 1)
        buildingsWanted[BuildingType::PigFarm] = GetNumBuildings(BuildingType::Slaughterhouse) + 1;*/

    // pig farms

    while(buildingsWanted[BuildingType::PigFarm] != GetNumBuildings(BuildingType::Farm) / 4)
        buildingsWanted[BuildingType::PigFarm] = GetNumBuildings(BuildingType::Farm) / 4;

    /* buildingsWanted[BuildingType::Slaughterhouse] = std::min(
       inventory.goods[GoodType::Cleaver] + inventory.people[Job::Butcher],
       GetNumBuildings(BuildingType::PigFarm));*/
    if(GetNumBuildings(BuildingType::PigFarm) != 0)
    {
        buildingsWanted[BuildingType::Slaughterhouse] = GetNumBuildings(BuildingType::PigFarm) - 1;
        if(buildingsWanted[BuildingType::Slaughterhouse] == 0)
            buildingsWanted[BuildingType::Slaughterhouse] = 1;
    }

    buildingsWanted[BuildingType::Well] = buildingsWanted[BuildingType::Bakery] + buildingsWanted[BuildingType::PigFarm]
                                          + buildingsWanted[BuildingType::DonkeyBreeder]
                                          + buildingsWanted[BuildingType::Brewery];

    /* if(aijh.ggs.isEnabled(AddonId::EXHAUSTIBLE_WATER))
     {
         float well = static_cast<float>(buildingsWanted[BuildingType::Well]);
         well *= 2.0f;
         buildingsWanted[BuildingType::Well] = static_cast<int>(well);
     }*/
    /* buildingsWanted[BuildingType::Farm] =
      std::min<unsigned>(inventory.goods[GoodType::Scythe] + inventory.people[Job::Farmer], foodusers + 3);*/
    buildingsWanted[BuildingType::Farm] = foodusers + 3;

    buildingsWanted[BuildingType::DonkeyBreeder] = 2;

    if(inventory.goods[GoodType::PickAxe] + inventory.people[Job::Miner] < 3)
    {
        // almost out of new pickaxes and miners - emergency program: get coal,iron,smelter&metalworks
        buildingsWanted[BuildingType::CoalMine] = 1;
        buildingsWanted[BuildingType::IronMine] = 1;
        buildingsWanted[BuildingType::GoldMine] = 0;
        buildingsWanted[BuildingType::Ironsmelter] = 1;
        buildingsWanted[BuildingType::Metalworks] = 1;
        buildingsWanted[BuildingType::Armory] = 0;
        buildingsWanted[BuildingType::GraniteMine] = 0;
        buildingsWanted[BuildingType::Mint] = 0;
    } else // more than 2 miners
    {
        // coal mine count now depends on iron & gold not linked to food or material supply - might have to add a
        // material check if this makes problems
        if(GetNumBuildings(BuildingType::IronMine) > 0)
            buildingsWanted[BuildingType::CoalMine] =
              GetNumBuildings(BuildingType::IronMine) * 2 - 1 + GetNumBuildings(BuildingType::GoldMine);
        else
            buildingsWanted[BuildingType::CoalMine] = std::max(GetNumBuildings(BuildingType::GoldMine), 1u);
        // more mines planned than food available? -> limit mines
        if(buildingsWanted[BuildingType::CoalMine] > 2
           && buildingsWanted[BuildingType::CoalMine] * 2
                > GetNumBuildings(BuildingType::Farm) + GetNumBuildings(BuildingType::Fishery) + 1)
            buildingsWanted[BuildingType::CoalMine] =
              (GetNumBuildings(BuildingType::Farm) + GetNumBuildings(BuildingType::Fishery)) / 2 + 2;
        if(GetNumBuildings(BuildingType::Farm) > 7) // quite the empire just scale mines with farms
        {
            if(aijh.ggs.isEnabled(
                 AddonId::INEXHAUSTIBLE_MINES)) // inexhaustible mines? -> more farms required for each mine
                buildingsWanted[BuildingType::IronMine] =
                  std::min(GetNumBuildings(BuildingType::Ironsmelter) + 1, GetNumBuildings(BuildingType::Farm) * 2 / 5);
            else
                buildingsWanted[BuildingType::IronMine] =
                  std::min(GetNumBuildings(BuildingType::Farm) / 2, GetNumBuildings(BuildingType::Ironsmelter) + 1);
            /* buildingsWanted[BuildingType::GoldMine] =
               (GetNumBuildings(BuildingType::Mint) > 0) ?
                 GetNumBuildings(BuildingType::Ironsmelter) > 6 && GetNumBuildings(BuildingType::Mint) > 1 ?
                 GetNumBuildings(BuildingType::Ironsmelter) > 10 ? 4 : 3 :
                 2 :
                 1;*/

            buildingsWanted[BuildingType::GoldMine] = GetNumBuildings(BuildingType::Ironsmelter) / 2;

            if(aijh.ggs.isEnabled(AddonId::CHARBURNER)
               && (buildingsWanted[BuildingType::CoalMine] > GetNumBuildings(BuildingType::CoalMine) + 4))
            {
                resourcelimit = inventory.people[Job::CharBurner] + inventory.goods[GoodType::Shovel] + 1;
                buildingsWanted[BuildingType::Charburner] = std::min<unsigned>(
                  std::min(buildingsWanted[BuildingType::CoalMine] - (GetNumBuildings(BuildingType::CoalMine) + 1), 3u),
                  resourcelimit);
            }
        } else
        {
            // probably still limited in food supply go up to 4 coal 1 gold 2 iron (gold+coal->coin,
            // iron+coal->tool, iron+coal+coal->weapon)
            unsigned numFoodProducers =
              GetNumBuildings(BuildingType::Bakery) + GetNumBuildings(BuildingType::Slaughterhouse)
              + GetNumBuildings(BuildingType::Hunter) + GetNumBuildings(BuildingType::Fishery);
            buildingsWanted[BuildingType::IronMine] =
              (inventory.people[Job::Miner] + inventory.goods[GoodType::PickAxe]
                 > GetNumBuildings(BuildingType::CoalMine) + GetNumBuildings(BuildingType::GoldMine) + 1
               && numFoodProducers > 4) ?
                2 :
                1;
            buildingsWanted[BuildingType::GoldMine] = (inventory.people[Job::Miner] > 2) ? 1 : 0;
            resourcelimit = inventory.people[Job::CharBurner] + inventory.goods[GoodType::Shovel];
            if(aijh.ggs.isEnabled(AddonId::CHARBURNER)
               && (GetNumBuildings(BuildingType::CoalMine) < 1
                   && (GetNumBuildings(BuildingType::IronMine) + GetNumBuildings(BuildingType::GoldMine) > 0)))
                buildingsWanted[BuildingType::Charburner] = std::min(1, resourcelimit);
        }
        if(GetNumBuildings(BuildingType::Quarry) + 1 < buildingsWanted[BuildingType::Quarry]
           && aijh.AmountInStorage(GoodType::Stones) < 100) // no quarry and low stones -> try granitemines.
        {
            if(inventory.people[Job::Miner] > 6)
            {
                buildingsWanted[BuildingType::GraniteMine] = std::min<int>(
                  militaryPower / 10 + 1, // limit granite mines to military / 10
                  std::max<int>(buildingsWanted[BuildingType::Quarry] - GetNumBuildings(BuildingType::Quarry), 1));
            } else
                buildingsWanted[BuildingType::GraniteMine] = 1;
        } else
            buildingsWanted[BuildingType::GraniteMine] = 0;
    }

    // always 1 granite mine in use
    if(GetNumBuildings(BuildingType::GraniteMine) <= 1)
    {
        buildingsWanted[BuildingType::GraniteMine] = 2;
    }

    // more charburners
    if(buildingsWanted[BuildingType::CoalMine] > 2)
    {
        if(GetNumBuildings(BuildingType::Farm) > 6) // char burners after we got a few farms going
            buildingsWanted[BuildingType::Charburner] = buildingsWanted[BuildingType::CoalMine] / 2u;
    }

    // Only build catapults if we have more than 5 military blds and 50 stones. Then reserve 4 stones per
    // catapult but do not build more than 4 additions catapults Note: This is a max amount. A catapult is only
    // placed if reasonable
    buildingsWanted[BuildingType::Catapult] =
      militaryPower <= 5 || inventory.goods[GoodType::Stones] < 50 ?
        0 :
        std::min((inventory.goods[GoodType::Stones] - 50) / 4, GetNumBuildings(BuildingType::Catapult) + 4);

    if(aijh.ggs.GetMaxMilitaryRank() == 0)
    {
        buildingsWanted[BuildingType::GoldMine] = 0; // max rank is 0 = private / recruit ==> gold is useless!
    }
}

int BuildingPlanner::GetNumAdditionalBuildingsWanted(BuildingType type) const
{
    return static_cast<int>(buildingsWanted[type]) - static_cast<int>(GetNumBuildings(type));
}

bool BuildingPlanner::WantMoreMilitaryBlds(const AIPlayerJH& aijh) const
{
    if(GetNumMilitaryBldSites() >= GetNumMilitaryBlds() + 3)
        return false;
    if(expansionRequired)
        return true;
    if(GetNumBuildings(BuildingType::Sawmill) > 0)
        return true;
    if(aijh.player.GetInventory().goods[GoodType::Boards] > 30 && GetNumBuildingSites(BuildingType::Sawmill) > 0)
        return true;
    return (GetNumMilitaryBlds() + GetNumMilitaryBldSites() > 0);
}

} // namespace AIJH
