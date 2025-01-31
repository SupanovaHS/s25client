// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "CollisionDetection.h"
#include "GameWithLuaAccess.h"
#include "Loader.h"
#include "WindowManager.h"
#include "controls/ctrlButton.h"
#include "controls/ctrlImage.h"
#include "controls/ctrlMultiline.h"
#include "ingameWindows/iwMissionStatement.h"
#include "ingameWindows/iwMsgbox.h"
#include "ogl/glFont.h"
#include "uiHelper/uiHelpers.hpp"

BOOST_FIXTURE_TEST_SUITE(LuaGUITestSuite, LuaTestsFixture)

BOOST_AUTO_TEST_CASE(MissionStatement)
{
    uiHelper::initGUITests();

    // Set player
    MOCK_EXPECT(localGameState.GetPlayerId).returns(1);

    BOOST_TEST_REQUIRE(!WINDOWMANAGER.GetTopMostWindow());

    // Show mission to non-existing player
    executeLua("rttr:MissionStatement(99, 'Title', 'Text')");
    BOOST_TEST_REQUIRE(!WINDOWMANAGER.GetTopMostWindow());

    // Show mission to other player
    executeLua("rttr:MissionStatement(0, 'Title', 'Text')");
    BOOST_TEST_REQUIRE(!WINDOWMANAGER.GetTopMostWindow());

    // Show it to us
    executeLua("rttr:MissionStatement(1, 'Title', 'Text')");
    const auto* wnd = dynamic_cast<const iwMissionStatement*>(WINDOWMANAGER.GetTopMostWindow());
    BOOST_TEST_REQUIRE(wnd);
    BOOST_TEST_REQUIRE(wnd->IsActive());
    BOOST_TEST_REQUIRE(wnd->GetTitle() == "Title");

    // double windows stack
    executeLua("rttr:MissionStatement(1, 'Title2', 'Text')");
    const auto* wnd2 = dynamic_cast<const iwMissionStatement*>(WINDOWMANAGER.GetTopMostWindow());
    BOOST_TEST_REQUIRE(wnd2);
    // Other window still on top
    BOOST_TEST_REQUIRE(wnd2 == wnd);
    // Close first wnd
    WINDOWMANAGER.Close(wnd);
    // 2nd shows
    wnd2 = dynamic_cast<const iwMissionStatement*>(WINDOWMANAGER.GetTopMostWindow());
    BOOST_TEST_REQUIRE(wnd2);
    BOOST_TEST_REQUIRE(wnd2 != wnd);
    BOOST_TEST_REQUIRE(wnd2->GetTitle() == "Title2");
    // Close wnd
    WINDOWMANAGER.Close(wnd2);
    BOOST_TEST_REQUIRE(!WINDOWMANAGER.GetTopMostWindow());

    // No image
    executeLua("rttr:MissionStatement(1, 'Title', 'Text', IM_NONE)");
    wnd = dynamic_cast<const iwMissionStatement*>(WINDOWMANAGER.GetTopMostWindow());
    BOOST_TEST_REQUIRE(wnd);
    BOOST_TEST_REQUIRE(wnd->IsActive());
    BOOST_TEST_REQUIRE(wnd->GetTitle() == "Title");
    BOOST_TEST_REQUIRE(wnd->GetCtrls<ctrlImage>().empty());
    WINDOWMANAGER.Close(wnd);
    // Non-default image
    executeLua("rttr:MissionStatement(1, 'Title', 'Text', IM_AVATAR10)");
    wnd = dynamic_cast<const iwMissionStatement*>(WINDOWMANAGER.GetTopMostWindow());
    BOOST_TEST_REQUIRE(wnd);
    BOOST_TEST_REQUIRE(wnd->IsActive());
    BOOST_TEST_REQUIRE(wnd->GetTitle() == "Title");
    BOOST_TEST_REQUIRE(!wnd->GetCtrls<ctrlImage>().empty());
    WINDOWMANAGER.Close(wnd);
    // Invalid image
    executeLua("rttr:MissionStatement(1, 'Title', 'Text', 999999)");
    wnd = dynamic_cast<const iwMissionStatement*>(WINDOWMANAGER.GetTopMostWindow());
    BOOST_TEST_REQUIRE(wnd);
    BOOST_TEST_REQUIRE(wnd->IsActive());
    BOOST_TEST_REQUIRE(wnd->GetTitle() == "Title");
    BOOST_TEST_REQUIRE(wnd->GetCtrls<ctrlImage>().empty());
    WINDOWMANAGER.Close(wnd);
}

BOOST_AUTO_TEST_CASE(MessageBoxTest)
{
    uiHelper::initGUITests();
    // Default Info
    executeLua("rttr:MsgBox('Title', string.rep('Text\\n', 15))");
    const auto* wnd = dynamic_cast<const iwMsgbox*>(WINDOWMANAGER.GetTopMostWindow());
    BOOST_TEST_REQUIRE(wnd);
    BOOST_TEST_REQUIRE(wnd->IsActive());
    BOOST_TEST_REQUIRE(wnd->GetTitle() == "Title");
    BOOST_TEST_REQUIRE(!wnd->GetCtrls<ctrlImage>().empty());
    BOOST_TEST_REQUIRE(wnd->GetCtrls<ctrlImage>().front()->GetImage()
                       == LOADER.GetImageN("io", rttr::enum_cast(MsgboxIcon::ExclamationGreen)));
    BOOST_TEST_REQUIRE(!wnd->GetCtrls<ctrlButton>().empty());
    // 15 lines of text, button must be below
    BOOST_TEST_REQUIRE(wnd->GetCtrls<ctrlButton>().front()->GetPos().y > 15 * NormalFont->getHeight());
    // And window higher than button
    BOOST_TEST_REQUIRE(static_cast<int>(wnd->GetIwSize().y) > wnd->GetCtrls<ctrlButton>().front()->GetPos().y);
    WINDOWMANAGER.Close(wnd);
    // Error
    executeLua("rttr:MsgBox('Title', 'Text', true)");
    wnd = dynamic_cast<const iwMsgbox*>(WINDOWMANAGER.GetTopMostWindow());
    BOOST_TEST_REQUIRE(wnd);
    BOOST_TEST_REQUIRE(wnd->IsActive());
    BOOST_TEST_REQUIRE(wnd->GetTitle() == "Title");
    BOOST_TEST_REQUIRE(!wnd->GetCtrls<ctrlImage>().empty());
    BOOST_TEST_REQUIRE(wnd->GetCtrls<ctrlImage>().front()->GetImage()
                       == LOADER.GetImageN("io", rttr::enum_cast(MsgboxIcon::ExclamationRed)));
    WINDOWMANAGER.Close(wnd);
    // MsgBoxEx
    executeLua("rttr:MsgBoxEx('Title', 'Text', 'io', 100)");
    wnd = dynamic_cast<const iwMsgbox*>(WINDOWMANAGER.GetTopMostWindow());
    BOOST_TEST_REQUIRE(wnd);
    BOOST_TEST_REQUIRE(wnd->IsActive());
    BOOST_TEST_REQUIRE(wnd->GetTitle() == "Title");
    BOOST_TEST_REQUIRE(!wnd->GetCtrls<ctrlImage>().empty());
    BOOST_TEST_REQUIRE(wnd->GetCtrls<ctrlImage>().front()->GetImage() == LOADER.GetImageN("io", 100));
    WINDOWMANAGER.Close(wnd);
    // MsgBoxEx with position
    executeLua("rttr:MsgBoxEx('Title', 'Text', 'io', 101, 500, 200)");
    wnd = dynamic_cast<const iwMsgbox*>(WINDOWMANAGER.GetTopMostWindow());
    BOOST_TEST_REQUIRE(wnd);
    BOOST_TEST_REQUIRE(wnd->IsActive());
    BOOST_TEST_REQUIRE(wnd->GetTitle() == "Title");
    BOOST_TEST_REQUIRE(!wnd->GetCtrls<ctrlImage>().empty());
    const ctrlImage* img = wnd->GetCtrls<ctrlImage>().front();
    const ITexture* actImg = img->GetImage();
    const Position imgPos = img->GetPos();
    BOOST_TEST_REQUIRE(actImg == LOADER.GetImageN("io", 101));
    BOOST_TEST_REQUIRE(imgPos.x == 500);
    BOOST_TEST_REQUIRE(imgPos.y == 200);
    // Window must be bigger than image pos+size
    Position imgEndPos(imgPos + actImg->GetSize() - actImg->GetOrigin());
    BOOST_TEST_REQUIRE(static_cast<int>(wnd->GetSize().x) > imgEndPos.x); //-V807
    BOOST_TEST_REQUIRE(static_cast<int>(wnd->GetSize().y) > imgEndPos.y);
    const ctrlButton* bt = wnd->GetCtrls<ctrlButton>().front();
    // button must be below start of image
    BOOST_TEST_REQUIRE(bt->GetPos().y > imgPos.y - actImg->GetOrigin().y); //-V807
    // and centered
    BOOST_TEST_REQUIRE(bt->GetPos().x <= static_cast<int>(wnd->GetSize().x / 2));
    BOOST_TEST_REQUIRE(bt->GetPos().x > static_cast<int>(wnd->GetSize().x / 2 - bt->GetSize().x));

    // Left, right, top, bottom
    const std::vector<DrawPoint> imgPts{DrawPoint(30, 30), DrawPoint(300, 30), DrawPoint(150, 30), DrawPoint(150, 300)};
    for(const auto& imgPt : imgPts)
    {
        const_cast<iwMsgbox*>(wnd)->MoveIcon(imgPt);
        Rect imgRect(img->GetPos() - actImg->GetOrigin(), actImg->GetSize());
        // Image must be in wnd
        BOOST_TEST_REQUIRE(static_cast<int>(wnd->GetSize().x) > imgRect.right);
        BOOST_TEST_REQUIRE(static_cast<int>(wnd->GetSize().y) > imgRect.bottom);
        // Not over button or text
        BOOST_TEST_REQUIRE(!DoRectsIntersect(imgRect, bt->GetDrawRect()));
        const ctrlMultiline* text = wnd->GetCtrls<ctrlMultiline>().front();
        BOOST_TEST_REQUIRE(!DoRectsIntersect(imgRect, text->GetDrawRect()));
    }
}

BOOST_AUTO_TEST_SUITE_END()
