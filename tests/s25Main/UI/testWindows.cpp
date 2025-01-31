// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "WindowManager.h"
#include "controls/ctrlButton.h"
#include "controls/ctrlGroup.h"
#include "controls/ctrlMultiline.h"
#include "desktops/Desktop.h"
#include "ingameWindows/iwVictory.h"
#include "uiHelper/uiHelpers.hpp"
#include <turtle/mock.hpp>
#include <boost/test/unit_test.hpp>

//-V:MOCK_METHOD:813
//-V:MOCK_EXPECT:807

BOOST_FIXTURE_TEST_SUITE(Windows, uiHelper::Fixture)

BOOST_AUTO_TEST_CASE(Victory)
{
    std::vector<std::string> winnerNames;
    winnerNames.push_back("FooName");
    winnerNames.push_back("BarNameBaz");
    const iwVictory wnd(winnerNames);
    // 2 buttons
    BOOST_TEST_REQUIRE(wnd.GetCtrls<ctrlButton>().size() == 2u);
    // Find a text field containing all winner names
    const auto txts = wnd.GetCtrls<ctrlMultiline>();
    bool found = false;
    for(const ctrlMultiline* txt : txts)
    {
        if(txt->GetNumLines() != winnerNames.size())
            continue; // LCOV_EXCL_LINE
        bool curFound = true;
        for(unsigned i = 0; i < winnerNames.size(); i++)
        {
            curFound &= txt->GetLine(i) == winnerNames[i];
        }
        found |= curFound;
    }
    BOOST_TEST_REQUIRE(found);
}

namespace {
/* clang-format off */
    MOCK_BASE_CLASS(TestWindow, Window)
    {
    public:
        TestWindow(Window* parent, unsigned id, const DrawPoint& position): Window(parent, id, position) {}
        MOCK_METHOD(Msg_PaintBefore, 0)
        MOCK_METHOD(Msg_PaintAfter, 0)
        MOCK_METHOD(Draw_, 0, void())
    };
/* clang-format on */
} // namespace

BOOST_AUTO_TEST_CASE(DrawOrder)
{
    Desktop* dsk = WINDOWMANAGER.GetCurrentDesktop();
    std::vector<TestWindow*> wnds;
    // Top level controls
    for(int i = 0; i < 3; i++)
    {
        wnds.push_back(new TestWindow(dsk, static_cast<unsigned>(wnds.size()), DrawPoint(0, 0)));
        dsk->AddCtrl(wnds.back());
    }
    // Some groups with own controls
    for(int i = 0; i < 3; i++)
    {
        ctrlGroup* grp = dsk->AddGroup(100 + i);
        for(int i = 0; i < 3; i++)
        {
            wnds.push_back(new TestWindow(dsk, static_cast<unsigned>(wnds.size()), DrawPoint(0, 0)));
            grp->AddCtrl(wnds.back());
        }
    }
    mock::sequence s;
    // Note: Actually order of calls to controls is undefined but in practice matches the IDs
    for(TestWindow* wnd : wnds)
        MOCK_EXPECT(wnd->Msg_PaintBefore).once().in(s);
    for(TestWindow* wnd : wnds)
        MOCK_EXPECT(wnd->Draw_).once().in(s);
    for(TestWindow* wnd : wnds)
        MOCK_EXPECT(wnd->Msg_PaintAfter).once().in(s);
    WINDOWMANAGER.Draw();
    mock::verify();
}

BOOST_AUTO_TEST_SUITE_END()
