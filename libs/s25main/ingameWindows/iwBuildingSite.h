// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "IngameWindow.h"

#include "world/GameWorld.h"


class noBuildingSite;
class GameWorldView;

class iwBuildingSite : public IngameWindow
{
public:
    iwBuildingSite(GameWorldView& gwv, const noBuildingSite* buildingsite, GameWorldBase& world);

protected:
    void Msg_ButtonClick(unsigned ctrl_id) override;
    void Msg_PaintAfter() override;
    void Msg_PaintBefore() override;

private:
    GameWorldView& gwv;
    const noBuildingSite* buildingsite;
    GameWorldBase& world;
    /// List of all buildings

};
