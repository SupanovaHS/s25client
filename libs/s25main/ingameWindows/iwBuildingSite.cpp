// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "iwBuildingSite.h"

#include "GamePlayer.h"
#include "Loader.h"
#include "WindowManager.h"
#include "buildings/noBuildingSite.h"
#include "iwDemolishBuilding.h"
#include "iwHelp.h"
#include "ogl/FontStyle.h"
#include "ogl/glArchivItem_Bitmap.h"
#include "controls/ctrlGroup.h"
#include "controls/ctrlText.h"
#include "world/GameWorldBase.h"
#include "world/GameWorldView.h"
#include "world/GameWorldViewer.h"
#include "world/MapBase.h"
#include "gameData/BuildingConsts.h"
#include "gameData/const_gui_ids.h"

iwBuildingSite::iwBuildingSite(GameWorldView& gwv, const noBuildingSite* const buildingsite, GameWorldBase& world)
    : IngameWindow(CGI_BUILDING + MapBase::CreateGUIID(buildingsite->GetPos()), IngameWindow::posAtMouse,
                   Extent(226, 194), _(BUILDING_NAMES[buildingsite->GetBuildingType()]),
                   LOADER.GetImageN("resource", 41)),
      gwv(gwv), buildingsite(buildingsite), world(world)
{
    // Bild des Gebäudes
    AddImage(0, DrawPoint(113, 130), &buildingsite->GetBuildingImage());
    // Gebäudename
    AddText(1, DrawPoint(113, 44), _("Order of building site"), COLOR_YELLOW, FontStyle::CENTER, NormalFont);

    // Hilfe
    AddImageButton(2, DrawPoint(16, 147), Extent(30, 32), TextureColor::Grey, LOADER.GetImageN("io", 225), _("Help"));
    // Gebäude abbrennen
    AddImageButton(3, DrawPoint(50, 147), Extent(34, 32), TextureColor::Grey, LOADER.GetImageN("io", 23),
                   _("Demolish house"));

    // "Gehe Zu Ort"
    AddImageButton(4, DrawPoint(179, 147), Extent(30, 32), TextureColor::Grey, LOADER.GetImageN("io", 107),
                   _("Go to place"));
    // Front of queue
    AddImageButton(5, DrawPoint(179, 58), Extent(30, 20), TextureColor::Grey, LOADER.GetImageN("io", 215),
                   _("Move to front of build queue"));
    // up one
    AddImageButton(6, DrawPoint(179, 78), Extent(30, 20), TextureColor::Grey, LOADER.GetImageN("io", 33),
                   _("Move up queue"));
    // down one
    AddImageButton(7, DrawPoint(179, 107), Extent(30, 20), TextureColor::Grey, LOADER.GetImageN("io", 34),
                   _("Move down queue"));
    // bottom of queue
    AddImageButton(8, DrawPoint(179, 127), Extent(30, 20), TextureColor::Grey, LOADER.GetImageN("io", 216),
                   _("Move to bottom of build queue"));
    // show current position in build queue
    AddText(9, DrawPoint(193,109), "", COLOR_YELLOW, FontStyle::CENTER | FontStyle::BOTTOM,SmallFont);
}
void iwBuildingSite::Msg_PaintBefore()
{
    IngameWindow::Msg_PaintBefore();
    int position = 1;

    auto world = buildingsite->GetWorld();
    auto localPlayer = world->GetPlayerptr(gwv.GetViewer().GetPlayer().GetPlayerId());
    auto buildingRegister = localPlayer->GetBuildingRegisterptr();
    auto buildingList = buildingRegister->GetBuildingSitesptr();
    auto buildingSiteIter = std::find(buildingList.begin(), buildingList.end(), buildingsite);

    // find our position
    for(auto it : buildingList)
    {
        if(it == buildingsite)
            break;
        ++position;
    }

    GetCtrl<ctrlText>(9)->SetText(std::to_string(position));
}

void iwBuildingSite::Msg_ButtonClick(const unsigned ctrl_id)
{
    auto world = buildingsite->GetWorld();
    auto localPlayer = world->GetPlayerptr(gwv.GetViewer().GetPlayer().GetPlayerId());
    auto buildingRegister = localPlayer->GetBuildingRegisterptr();
    auto buildingList = buildingRegister->GetBuildingSitesptr();
    auto buildingSiteIter = std::find(buildingList.begin(), buildingList.end(), buildingsite);

    switch(ctrl_id)
    {
        case 2: // Hilfe
        {
            WINDOWMANAGER.ReplaceWindow(
              std::make_unique<iwHelp>(_(BUILDING_HELP_STRINGS[buildingsite->GetBuildingType()])));
        }
        break;
        case 3: // Gebäude abbrennen
        {
            // Abreißen?
            Close();
            WINDOWMANAGER.Show(std::make_unique<iwDemolishBuilding>(gwv, buildingsite));
        }
        break;
        case 4: // "Gehe Zu Ort"
        {
            gwv.MoveToMapPt(buildingsite->GetPos());
        }
        break;
        case 5: // front of queue
            {
            localPlayer->MoveBuildingSiteFront(*buildingSiteIter);
            }
        break;
        case 6: // up one
            {
             localPlayer->MoveBuildingSiteUp(*buildingSiteIter);
            }
        break;
        case 7: // down one
            {
             localPlayer->MoveBuildingSiteDown(*buildingSiteIter);
            }
        break;
        case 8: // bottom of queue
            {
            localPlayer->MoveBuildingSiteBottom(*buildingSiteIter);
            }
        break;

    }
    int foo = 0;
}

void iwBuildingSite::Msg_PaintAfter()
{
    IngameWindow::Msg_PaintAfter();
    // Baukosten zeichnen
    DrawPoint curPos = GetDrawPos() + DrawPoint(GetSize().x / 2, 60);
    for(unsigned char i = 0; i < 2; ++i)
    {
        unsigned wares_count = 0;
        unsigned wares_delivered = 0;
        unsigned wares_used = 0;

        if(i == 0)
        {
            wares_count = BUILDING_COSTS[buildingsite->GetBuildingType()].boards;
            wares_used = buildingsite->getUsedBoards();
            wares_delivered = buildingsite->getBoards() + wares_used;
        } else
        {
            wares_count = BUILDING_COSTS[buildingsite->GetBuildingType()].stones;
            wares_used = buildingsite->getUsedStones();
            wares_delivered = buildingsite->getStones() + wares_used;
        }

        if(wares_count == 0)
            break;

        // "Schwarzer Rahmen"
        DrawPoint waresPos = curPos - DrawPoint(24 * wares_count / 2, 0);
        DrawRectangle(Rect(waresPos, Extent(24 * wares_count, 24)), 0x80000000);
        waresPos += DrawPoint(12, 12);

        // Die Waren
        for(unsigned char z = 0; z < wares_count; ++z)
        {
            LOADER.GetWareTex(i == 0 ? GoodType::Boards : GoodType::Stones)
              ->DrawFull(waresPos, (z < wares_delivered ? 0xFFFFFFFF : 0xFF404040));

            // Hammer wenn Ware verbaut
            if(z < wares_used)
                LOADER.GetWareTex(GoodType::Hammer)->DrawFull(waresPos);
            waresPos.x += 24;
        }
        curPos.y += 29;
    }
}
