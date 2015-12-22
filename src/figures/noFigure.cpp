//
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

///////////////////////////////////////////////////////////////////////////////
// Header
#include "defines.h"
#include "figures/noFigure.h"

#include "Loader.h"
#include "nodeObjs/noRoadNode.h"
#include "EventManager.h"
#include "MapGeometry.h"
#include "gameData/MapConsts.h"
#include "nofCarrier.h"
#include "nodeObjs/noSkeleton.h"

#include "buildings/nobBaseWarehouse.h"
#include "buildings/nobHarborBuilding.h"
#include "gameData/DoorConsts.h"
#include "macros.h"
#include "GameClient.h"
#include "GameClientPlayer.h"

#include "ogl/glSmartBitmap.h"

#include "Random.h"

#include "SerializedGameData.h"
#include "gameData/JobConsts.h"
#include "gameData/GameConsts.h"

#include "Log.h"

///////////////////////////////////////////////////////////////////////////////
// Makros / Defines
#if defined _WIN32 && defined _DEBUG && defined _MSC_VER
#define new new(_NORMAL_BLOCK, THIS_FILE, __LINE__)
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


const RoadSegment noFigure::emulated_wanderroad(RoadSegment::RT_NORMAL, 0, 0, std::vector<unsigned char>(0, 0));
/// Welche Strecke soll minimal und maximal zurückgelegt werden beim Rumirren, bevor eine Flagge gesucht wird
const unsigned short WANDER_WAY_MIN = 20;
const unsigned short WANDER_WAY_MAX = 40;
/// Versuche, eine Flagge zu finden, bis er stirbt beim Rumirren
const unsigned short WANDER_TRYINGS = 3;
// Größe des Rechtecks um den Punkt, wo er die Flaggen sucht beim Rumirren
const unsigned short WANDER_RADIUS = 10;
/// Dasselbe nochmal für Soldaten
const unsigned short WANDER_TRYINGS_SOLDIERS = 6;
const unsigned short WANDER_RADIUS_SOLDIERS = 15;




noFigure::noFigure(const Job job, const MapPoint pos, const unsigned char player, noRoadNode* const goal)
    :   noMovable(NOP_FIGURE, pos), fs(FS_GOTOGOAL), job_(job), player(player), cur_rs(0),
        rs_pos(0), rs_dir(0), on_ship(false), goal_(goal), waiting_for_free_node(false),
        flagPos_(0xFFFF, 0xFFFF), last_id(0xFFFFFFFF)

{
    //if(GetVisualRange())
    //  gwg->SetVisibilitiesAroundPoint(x,y,GetVisualRange(),player);

    // Haben wir ein Ziel?
    // Gehen wir in ein Lagerhaus? Dann dürfen wir da nicht unsere Arbeit ausführen, sondern
    // gehen quasi nach Hause von Anfang an aus
    if(goal && (goal->GetGOT() == GOT_NOB_HARBORBUILDING || goal->GetGOT() == GOT_NOB_STOREHOUSE
                || goal->GetGOT() == GOT_NOB_HQ))
        fs = FS_GOHOME;

}

noFigure::noFigure(const Job job, const MapPoint pos, const unsigned char player)
    :   noMovable(NOP_FIGURE, pos), fs(FS_JOB), job_(job), player(player), cur_rs(0),
        rs_pos(0), rs_dir(0), on_ship(false), goal_(0), waiting_for_free_node(false), last_id(0xFFFFFFFF)
{
    //f(GetVisualRange())
    //  gwg->SetVisibilitiesAroundPoint(x,y,GetVisualRange(),player);
}

void noFigure::Destroy_noFigure()
{
    assert(HasNoGoal());
    Destroy_noMovable();

    assert(!players->getElement(player)->IsDependentFigure(this));
}

void noFigure::Serialize_noFigure(SerializedGameData& sgd) const
{
    Serialize_noMovable(sgd);

    sgd.PushUnsignedChar(static_cast<unsigned char>(fs));
    sgd.PushUnsignedChar(static_cast<unsigned char>(job_));
    sgd.PushUnsignedChar(player);
    sgd.PushObject(cur_rs, true);
    sgd.PushUnsignedShort(rs_pos);
    sgd.PushBool(rs_dir);
    sgd.PushBool(on_ship);

    if(fs == FS_GOTOGOAL || fs == FS_GOHOME)
        sgd.PushObject(goal_, false);

    sgd.PushBool(waiting_for_free_node);

    if(fs == FS_WANDER)
    {
        sgd.PushUnsignedShort(wander_way);
        sgd.PushUnsignedShort(wander_tryings);
        sgd.PushMapPoint(flagPos_);
        sgd.PushUnsignedInt(flag_obj_id);
        sgd.PushUnsignedInt(burned_wh_id);
    }
}

noFigure::noFigure(SerializedGameData& sgd, const unsigned obj_id) : noMovable(sgd, obj_id),
    fs(FigureState(sgd.PopUnsignedChar())),
    job_(Job(sgd.PopUnsignedChar())),
    player(sgd.PopUnsignedChar()),
    cur_rs(sgd.PopObject<RoadSegment>(GOT_ROADSEGMENT)),
    rs_pos(sgd.PopUnsignedShort()),
    rs_dir(sgd.PopBool()),
    on_ship(sgd.PopBool()),
    last_id(0xFFFFFFFF)
{
    if(fs == FS_GOTOGOAL || fs == FS_GOHOME)
        goal_ = sgd.PopObject<noRoadNode>(GOT_UNKNOWN);
    else
        goal_ = NULL;

    waiting_for_free_node = sgd.PopBool();

    if(fs == FS_WANDER)
    {
        wander_way = sgd.PopUnsignedShort();
        wander_tryings = sgd.PopUnsignedShort();
        flagPos_ = sgd.PopMapPoint();
        flag_obj_id = sgd.PopUnsignedInt();
        burned_wh_id = sgd.PopUnsignedInt();
    }
}


void noFigure::ActAtFirst()
{
    // Je nach unserem Status bestimmte Dinge tun
    switch(fs)
    {
        default: break;
        case FS_GOTOGOAL: WalkToGoal(); break;
        case FS_JOB: StartWalking(4); break; // erstmal rauslaufen, darum kümmern sich dann die abgeleiteten Klassen
        case FS_GOHOME:
        {
            // Wenn ich gleich wieder nach Hause geschickt wurde und aus einem Lagerhaus rauskomme, gar nicht erst rausgehen!
            if(goal_->GetPos() == pos)
            {
                gwg->RemoveFigure(this, pos);
                assert(static_cast<nobBaseWarehouse*>(goal_));
                // Reset goal before re-adding to wh
                nobBaseWarehouse* wh = static_cast<nobBaseWarehouse*>(goal_);
                goal_ = NULL;
                wh->AddFigure(this);
            }
            else
                // ansonsten ganz normal rausgehen
                WalkToGoal();
        } break;
        case FS_WANDER: StartWalking(4); break; // erstmal rauslaufen, darum kümmern sich dann die Wander-Funktionen
    }
}


/// Gibt den Sichtradius dieser Figur zurück (0, falls nicht-spähend)
unsigned noFigure::GetVisualRange() const
{
    return 0;
}

/// Legt die Anfangsdaten für das Laufen auf Wegen fest
void noFigure::InitializeRoadWalking(const RoadSegment* const road, const unsigned short rs_pos, const bool rs_dir)
{
    this->cur_rs = road;
    this->rs_pos = rs_pos;
    this->rs_dir = rs_dir;
}

Point<int> noFigure::CalcFigurRelative() const
{
    Point<int> curPt  = Point<int>(gwg->GetTerrain(pos));
    Point<int> nextPt = Point<int>(gwg->GetTerrain(gwg->GetNeighbour(pos, GetCurMoveDir())));

    // Gehen wir über einen Kartenrand (horizontale Richung?)
    const int mapWidth = gwg->GetWidth() * TR_W;
    if(std::abs(curPt.x - nextPt.x) >= mapWidth / 2)
    {
        // So we need to get closer to nextPt
        if(curPt.x > nextPt.x)
            curPt.x -= mapWidth;
        else
            curPt.x += mapWidth;
    }
    // Und dasselbe für vertikale Richtung
    const int mapHeight = gwg->GetHeight() * TR_H;
    if(std::abs(curPt.y - nextPt.y) >= mapHeight / 2)
    {
        if(curPt.y > nextPt.y)
            curPt.y -= mapHeight;
        else
            curPt.y += mapHeight;
    }

    Point<int> result;

    const MapPoint nb = gwg->GetNeighbour(pos, 1);
    if(GetCurMoveDir() == 1 && (gwg->GetNO(nb)->GetType() == NOP_BUILDINGSITE || gwg->GetNO(nb)->GetType() == NOP_BUILDING))
    {
        noBaseBuilding* const bld = gwg->GetSpecObj<noBaseBuilding>(nb);
        nextPt += bld->GetDoorPoint();
        result = bld->GetDoorPoint();
    }else if(gwg->GetNO(pos)->GetType() == NOP_BUILDINGSITE || gwg->GetNO(pos)->GetType() == NOP_BUILDING)
    {
        noBaseBuilding* const bld = gwg->GetSpecObj<noBaseBuilding>(pos);
        curPt += bld->GetDoorPoint();
        result = bld->GetDoorPoint();
    }else
        result = Point<int>(0, 0);

    // Wenn die Träger runterlaufne, muss es andersrum sein, da die Träger dann immer vom OBEREN Punkt aus gezeichnet werden
    if(GetCurMoveDir() == 1 || GetCurMoveDir() == 2)
    {
        using std::swap;
        swap(curPt, nextPt);
    }

    return result + CalcRelative(curPt, nextPt);
}

void noFigure::StartWalking(const unsigned char newDir)
{
    assert(!(GetGOT() == GOT_NOF_PASSIVESOLDIER && fs == FS_JOB));

    assert(newDir <= 5);
    if(newDir > 5)
    {
        LOG.lprintf("WARNING: Bug detected (GF: %u). Please report this with the savegame and replay. noFigure::StartWalking: dir = %d\n", GAMECLIENT.GetGFNumber(), unsigned(newDir));
        return;
    }

    // Gehen wir in ein Gebäude?
    if(newDir == 1 && gwg->GetNO(gwg->GetNeighbour(pos, 1))->GetType() == NOP_BUILDING)
        gwg->GetSpecObj<noBuilding>(gwg->GetNeighbour(pos, 1))->OpenDoor(); // Dann die Tür aufmachen
    // oder aus einem raus?
    if(newDir == 4 && gwg->GetNO(pos)->GetType() == NOP_BUILDING)
        gwg->GetSpecObj<noBuilding>(pos)->OpenDoor(); // Dann die Tür aufmachen

    // Ist der Platz schon besetzt, wo wir hinlaufen wollen und laufen wir auf Straßen?
    if(!gwg->IsRoadNodeForFigures(gwg->GetNeighbour(pos, newDir), newDir) && cur_rs)
    {
        // Dann stehen bleiben!
        FaceDir(newDir);
        waiting_for_free_node = true;
        // Andere Figuren stoppen
        gwg->StopOnRoads(pos, newDir);
    }
    else
    {
        // Normal hinlaufen
        StartMoving(newDir, 20);
    }
}

/*void noFigure::StartWalkingFailedTrade(const unsigned char dir)
{
    assert(!(GetGOT() == GOT_NOF_PASSIVESOLDIER && fs == FS_JOB));

    assert(dir <= 5);
    if(dir > 5)
    {
        LOG.lprintf("WARNING: Bug detected (GF: %u). Please report this with the savegame and replay. noFigure::StartWalking: dir = %d\n", GAMECLIENT.GetGFNumber(), unsigned(dir));
        return;
    }

    // Gehen wir in ein Gebäude?
    if(dir == 1 && gwg->GetNO(gwg->GetXA(x,y,1),gwg->GetYA(x,y,1))->GetType() == NOP_BUILDING)
        gwg->GetSpecObj<noBuilding>(gwg->GetXA(x,y,1),gwg->GetYA(x,y,1))->OpenDoor(); // Dann die Tür aufmachen
    // oder aus einem raus?
    if(dir == 4 && gwg->GetNO(x,y)->GetType() == NOP_BUILDING)
        gwg->GetSpecObj<noBuilding>(x,y)->OpenDoor(); // Dann die Tür aufmachen

    // Ist der Platz schon besetzt, wo wir hinlaufen wollen und laufen wir auf Straßen?
    if(!gwg->IsRoadNodeForFigures(gwg->GetXA(x,y,dir),gwg->GetYA(x,y,dir),dir) &&
        cur_rs)
    {
        // Dann stehen bleiben!
        this->dir = dir;
        waiting_for_free_node = true;
        // Andere Figuren stoppen
        gwg->StopOnRoads(x,y,dir);
    }
    else
    {
        // Normal hinlaufen
        StartMoving(dir,20);
    }
}*/



void noFigure::DrawShadow(const int x, const int y, const unsigned char anistep, unsigned char dir)
{
    glArchivItem_Bitmap* bitmap = LOADER.GetMapImageN(900 + ( (dir + 3) % 6 ) * 8 + anistep);
    if(bitmap)
        bitmap->Draw(x, y, 0, 0, 0, 0, 0, 0, COLOR_SHADOW);
}

void noFigure::WalkFigure()
{
    // Tür hinter sich zumachen, wenn wir aus einem Gebäude kommen
    if(GetCurMoveDir() == 4 && gwg->GetNO(pos)->GetType() == NOP_BUILDING)
        gwg->GetSpecObj<noBuilding>(pos)->CloseDoor();

    Walk();

    if(cur_rs)
        ++rs_pos;


    // oder in eins reingegangen sind
    if(GetCurMoveDir() == 1 && gwg->GetNO(pos)->GetType() == NOP_BUILDING)
        gwg->GetSpecObj<noBuilding>(pos)->CloseDoor();

}


void noFigure::WalkToGoal()
{
    // Kein Ziel mehr --> Rumirren
    if(!goal_)
    {
        StartWandering();
        Wander();
        return;
    }

    // Straße abgelaufen oder noch gar keine Straße vorhanden?
    if(((cur_rs) ? (rs_pos == cur_rs->GetLength()) : true))
    {
        // Ziel erreicht?
        // Bei dem Träger können das beide Flaggen sein!
        MapPoint goal1, goal2;
        if(GetGOT() == GOT_NOF_CARRIER && fs == FS_GOTOGOAL)
        {
            goal1 = static_cast<nofCarrier*>(this)->GetFirstFlag() ?
                static_cast<nofCarrier*>(this)->GetFirstFlag()->GetPos() : MapPoint(0xFFFF, 0xFFFF);
            goal2 = static_cast<nofCarrier*>(this)->GetSecondFlag() ?
                static_cast<nofCarrier*>(this)->GetSecondFlag()->GetPos() : MapPoint(0xFFFF, 0xFFFF);
        }
        else
        {
            goal1 = goal_->GetPos();
            goal2 = MapPoint(0xFFFF, 0xFFFF);
        }

        if(goal1 == pos || goal2 == pos)
        {
            noRoadNode* goal = goal_;
            // Zeug nullen
            cur_rs = NULL;
            goal_ = NULL;
            rs_dir = 0;
            rs_pos = 0;
            if(fs == FS_GOHOME)
            {
                // Mann im Lagerhaus angekommen
                gwg->RemoveFigure(this, pos);
                static_cast<nobBaseWarehouse*>(goal)->AddFigure(this);
            }
            else
            {
                // abgeleiteter Klasse sagen, dass das Ziel erreicht wurde
                fs = FS_JOB;
                GoalReached();
            }

        }
        else
        {
            MapPoint next_harbor;
            // Neuen Weg berechnen
            noRoadNode* roadNode = gwg->GetSpecObj<noRoadNode>(pos);
            unsigned char route = roadNode ? gwg->FindHumanPathOnRoads(*roadNode, *goal_, NULL, &next_harbor) : 0xFF;
            // Kein Weg zum Ziel... nächstes Lagerhaus suchen
            if(route == 0xFF)
            {
                // Arbeisplatz oder Laghaus Bescheid sagen
                Abrogate();
                // Wir gehen jetzt nach Hause
                GoHome();
                // Evtl wurde kein Lagerhaus gefunden und wir sollen rumirren, dann tun wir das gleich
                if(fs == FS_WANDER)
                {
                    Wander();
                    return;
                }

                // Nach Hause laufen...
                WalkToGoal();
                return;
            }
            // Oder müssen wir das Schiff nehmen?
            else if(route == SHIP_DIR)
            {
                // Uns in den Hafen einquartieren
                noBase* nob = gwg->GetNO(pos);
                if(nob->GetGOT() != GOT_NOB_HARBORBUILDING)
                {
                    // Es gibt keinen Hafen mehr -> nach Hause gehen

                    // Arbeitsplatz oder Lagerhaus Bescheid sagen
                    Abrogate();
                    // Wir gehen jetzt nach Hause
                    GoHome();
                    // Evtl wurde kein Lagerhaus gefunden und wir sollen rumirren, dann tun wir das gleich
                    if(fs == FS_WANDER)
                    {
                        Wander();
                        return;
                    }

                    // Nach Hause laufen...
                    WalkToGoal();
                    return;
                }

                // Uns in den Hafen einquartieren
                cur_rs = NULL; // wir laufen nicht mehr auf einer Straße
                gwg->RemoveFigure(this, pos);
                static_cast<nobHarborBuilding*>(nob)->AddFigureForShip(this, next_harbor);

                return;
            }


            // Nächste Straße wollen, auf der man geht
            cur_rs = gwg->GetSpecObj<noRoadNode>(pos)->routes[route];
            StartWalking(route);
            rs_pos = 0;
            rs_dir = (gwg->GetSpecObj<noRoadNode>(pos) == cur_rs->GetF1()) ? false : true;
        }

    }
    else
    {
        StartWalking(cur_rs->GetDir(rs_dir, rs_pos));
    }
}

/*void noFigure::WalkToGoalFailedTrade()
{
    // Kein Ziel mehr --> Rumirren
    if(!goal)
    {
        StartWanderingFailedTrade();
        WanderFailedTrade();
        return;
    }

    // Straße abgelaufen oder noch gar keine Straße vorhanden?
    if(((cur_rs)?(rs_pos == cur_rs->GetLength()):true))
    {
        // Ziel erreicht?
        // Bei dem Träger können das beide Flaggen sein!
        unsigned short goal_x1, goal_y1, goal_x2=0xFFFF, goal_y2=0xFFFF;
        if(GetGOT() == GOT_NOF_CARRIER && fs == FS_GOTOGOAL)
        {
            goal_x1 = static_cast<nofCarrier*>(this)->GetFirstFlag() ?
                static_cast<nofCarrier*>(this)->GetFirstFlag()->GetX() : 0xFFFF;
            goal_y1 = static_cast<nofCarrier*>(this)->GetFirstFlag() ?
                static_cast<nofCarrier*>(this)->GetFirstFlag()->GetY() : 0xFFFF;
            goal_x2 = static_cast<nofCarrier*>(this)->GetSecondFlag() ?
                static_cast<nofCarrier*>(this)->GetSecondFlag()->GetX() : 0xFFFF;
            goal_y2 = static_cast<nofCarrier*>(this)->GetSecondFlag() ?
                static_cast<nofCarrier*>(this)->GetSecondFlag()->GetY() : 0xFFFF;
        }
        else
        {
            goal_x1 = goal->GetX();
            goal_y1 = goal->GetY();
        }

        if((goal_x1 == x && goal_y1 == y) || (goal_x2 == x && goal_y2 == y))
        {
            if(fs == FS_GOHOME)
            {
                // Mann im Lagerhaus angekommen     - this is what we hope happens ...
                gwg->GetPlayer(static_cast<nobBaseWarehouse*>(goal)->GetPlayer()).IncreaseInventoryJob(this->GetJobType(),1);
                static_cast<nobBaseWarehouse*>(goal)->AddFigure(this);
                gwg->RemoveFigure(this,x,y);
                // the ware we carried is lost because poc forgot to carry the information and is too lazy to fix it now ...

            }
            else
            {
                // Zeug nullen
                cur_rs = NULL;
                rs_dir = 0;
                rs_pos = 0;
                goal = NULL;


                // abgeleiteter Klasse sagen, dass das Ziel erreicht wurde
                fs = FS_JOB;
                GoalReached();
            }

        }
        else
        {
            MapPoint next_harbor;
            // Neuen Weg berechnen
            unsigned char route = gwg->FindHumanPathOnRoads(gwg->GetSpecObj<noRoadNode>(x,y),goal,NULL,&next_harbor);
            // Kein Weg zum Ziel... nächstes Lagerhaus suchen
            if(route == 0xFF)
            {
                // Arbeisplatz oder Laghaus Bescheid sagen
                Abrogate();
                // Wir gehen jetzt nach Hause
                GoHome();
                // Evtl wurde kein Lagerhaus gefunden und wir sollen rumirren, dann tun wir das gleich
                if(fs == FS_WANDER)
                {
                    WanderFailedTrade();
                    return;
                }

                // Nach Hause laufen...
                WalkToGoalFailedTrade();
                return;
            }
            // Oder müssen wir das Schiff nehmen? shouldnt happen for lost wanderers
            else if(route == SHIP_DIR)
            {
                // Uns in den Hafen einquartieren
                noBase * nob;
                if((nob=gwg->GetNO(x,y))->GetGOT() != GOT_NOB_HARBORBUILDING)
                {
                    // Es gibt keinen Hafen mehr -> nach Hause gehen

                    // Arbeisplatz oder Laghaus Bescheid sagen
                    Abrogate();
                    // Wir gehen jetzt nach Hause
                    GoHome();
                    // Evtl wurde kein Lagerhaus gefunden und wir sollen rumirren, dann tun wir das gleich
                    if(fs == FS_WANDER)
                    {
                        WanderFailedTrade();
                        return;
                    }

                    // Nach Hause laufen...
                    WalkToGoalFailedTrade();
                    return;
                }

                // Uns in den Hafen einquartieren
                cur_rs = NULL; // wir laufen nicht mehr auf einer Straße
                gwg->RemoveFigure(this,x,y);
                static_cast<nobHarborBuilding*>(nob)->AddFigureForShip(this,next_harbor);

                return;
            }


            // Nächste Straße wollen, auf der man geht
            cur_rs = gwg->GetSpecObj<noRoadNode>(x,y)->routes[route];
            StartWalking(route);
            rs_pos = 0;
            rs_dir = (gwg->GetSpecObj<noRoadNode>(x,y) == cur_rs->GetF1()) ? false : true;
        }

    }
    else
    {
        StartWalkingFailedTrade(cur_rs->GetDir(rs_dir,rs_pos));
    }
}*/



void noFigure::HandleEvent(const unsigned int id)
{
    // Bei ID = 0 ists ein Laufevent, bei allen anderen an abgeleitete Klassen weiterleiten
    if(id)
    {
        HandleDerivedEvent(id);
    }
    else
    {
        current_ev = NULL;
        WalkFigure();

        // Alte Richtung und Position für die Berechnung der Sichtbarkeiten merken
        unsigned char old_dir = GetCurMoveDir();

        MapPoint old_pos(pos);

        switch(fs)
        {
            case FS_GOHOME:
            case FS_GOTOGOAL:
            {
                WalkToGoal();
            } break;

            case FS_JOB:
            {
                Walked();
                break;
            }
            case FS_WANDER:
            {
                Wander();
                break;
            }
        }

        // Ggf. Sichtbereich testen
        if(GetVisualRange())
        {

            // Use old position (don't use this->x/y because it might be different now
            // Figure could be in a ship etc.)
            gwg->RecalcMovingVisibilities(old_pos, player, GetVisualRange(), old_dir, NULL);


            std::vector<noBase*> figures = gwg->GetDynamicObjectsFrom(old_pos);

            // Wenn Figur verschwunden ist, muss ihr ehemaliger gesamter Sichtbereich noch einmal
            // neue berechnet werden
            if(!helpers::contains(figures, this))
                CalcVisibilities(old_pos);
        }

    }
}

void noFigure::GoHome(noRoadNode* goal)
{
    if(on_ship)
    {
        // Wir befinden uns gerade an Deck, also einfach goal auf Null setzen und dann sehen wir, was so passiert
        this->goal_ = NULL;
        return;
    }
    // Nächstes Lagerhaus suchen
    else if(!goal)
    {
        // Wenn wir cur_rs == 0, dann hängen wir wahrscheinlich noch im Lagerhaus in der Warteschlange
        if(cur_rs == 0)
        {
            assert(gwg->GetNO(pos)->GetGOT() == GOT_NOB_HQ || //-V807
                   gwg->GetNO(pos)->GetGOT() == GOT_NOB_STOREHOUSE
                   || gwg->GetNO(pos)->GetGOT() == GOT_NOB_HARBORBUILDING);

            goal_ = NULL;
            gwg->GetSpecObj<nobBaseWarehouse>(pos)->CancelFigure(this);
            return;
        }
        else
            this->goal_ = gwg->GetPlayer(player).FindWarehouse((rs_dir) ? *cur_rs->GetF1() : *cur_rs->GetF2(), FW::Condition_StoreFigure, 0, true, &job_, false);
    }
    else
        this->goal_ = goal;

    if(this->goal_)
    {
        fs = FS_GOHOME;
        // Lagerhaus Bescheid sagen
        static_cast<nobBaseWarehouse*>(this->goal_)->AddDependentFigure(this);

        // Wenn wir stehen, zusätzlich noch loslaufen!
        if(waiting_for_free_node)
        {
            waiting_for_free_node = false;
            WalkToGoal();
            // anderen Leuten noch ggf Bescheid sagen
            gwg->RoadNodeAvailable(this->pos);
        }
    }
    else
    {
        // Kein Lagerhaus gefunden --> Rumirren
        StartWandering();
        cur_rs = NULL;
    }
}



void noFigure::StartWandering(const unsigned burned_wh_id)
{
    assert(HasNoGoal());
    fs = FS_WANDER;
    cur_rs = 0;
    rs_pos = 0;
    this->burned_wh_id = burned_wh_id;
    // eine bestimmte Strecke rumirren und dann eine Flagge suchen
    // 3x rumirren und eine Flagge suchen, wenn dann keine gefunden wurde, stirbt die Figur
    wander_way = WANDER_WAY_MIN + RANDOM.Rand(__FILE__, __LINE__, GetObjId(), WANDER_WAY_MAX - WANDER_WAY_MIN);
    // Soldaten sind härter im Nehmen
    bool is_soldier = (job_ >= JOB_PRIVATE && job_ <= JOB_GENERAL);
    wander_tryings = is_soldier ? WANDER_TRYINGS_SOLDIERS : WANDER_TRYINGS;

    // Wenn wir stehen, zusätzlich noch loslaufen!
    if(waiting_for_free_node)
    {
        waiting_for_free_node = false;
        Wander();
    }
}

/*void noFigure::StartWanderingFailedTrade(const unsigned burned_wh_id)
{
    fs = FS_WANDER;
    cur_rs = 0;
    goal = 0;
    rs_pos = 0;
    this->burned_wh_id = burned_wh_id;
    // eine bestimmte Strecke rumirren und dann eine Flagge suchen
    // 3x rumirren und eine Flagge suchen, wenn dann keine gefunden wurde, stirbt die Figur
    wander_way = WANDER_WAY_MIN + RANDOM.Rand(__FILE__,__LINE__,obj_id,WANDER_WAY_MAX-WANDER_WAY_MIN);
    // Soldaten sind härter im Nehmen
    bool is_soldier = (job >= JOB_PRIVATE && job <= JOB_GENERAL);
    wander_tryings = is_soldier ? WANDER_TRYINGS_SOLDIERS : WANDER_TRYINGS;

    // Wenn wir stehen, zusätzlich noch loslaufen!
    if(waiting_for_free_node)
    {
        waiting_for_free_node = false;
        WanderFailedTrade();
    }
}*/

struct Point2Flag{
    typedef noFlag* result_type;
    GameWorldBase& gwb;

    Point2Flag(GameWorldBase& gwb): gwb(gwb){}

    result_type operator()(const MapPoint pt, unsigned r) const
    {
        return gwb.GetSpecObj<noFlag>(pt);
    }
};

struct IsValidFlag{
    const unsigned playerId_;

    IsValidFlag(const unsigned playerId): playerId_(playerId){}

    bool operator()(const noFlag* const flag)
    {
        return flag && flag->GetPlayer() == playerId_;
    }
};

void noFigure::Wander()
{
    // Sind wir noch auf der Suche nach einer Flagge?
    if(wander_way == 0xFFFF)
    {
        // Wir laufen schon zur Flagge
        WanderToFlag();
        return;
    }

    // Ist es mal wieder an der Zeit, eine Flagge zu suchen?
    if(!wander_way)
    {
        // Soldaten sind härter im Nehmen
        const bool is_soldier = (job_ >= JOB_PRIVATE && job_ <= JOB_GENERAL);
        const unsigned short wander_radius = is_soldier ? WANDER_RADIUS_SOLDIERS : WANDER_RADIUS;

        // Flaggen sammeln und dann zufällig eine auswählen
        const std::vector<noFlag*> flags = gwg->GetPointsInRadius<0>(pos, wander_radius, Point2Flag(*gwg), IsValidFlag(player));

        unsigned best_way = 0xFFFFFFFF;
        noFlag const* best_flag = NULL;

        for(std::vector<noFlag*>::const_iterator it = flags.begin(); it != flags.end(); ++it)
        {
            // Ist das ein Flüchtling aus einem abgebrannten Lagerhaus?
            if(burned_wh_id != 0xFFFFFFFF)
            {
                // Dann evtl gucken, ob anderen Mitglieder schon gesagt haben, dass die Flagge nicht zugänglich ist
                if((*it)->IsImpossibleForBWU(burned_wh_id))
                {
                    // Dann können wir die Flagge überspringen
                    continue;
                }
            }

            // würde die die bisher beste an Weg unterbieten?
            unsigned way = gwg->CalcDistance(pos, (*it)->GetPos());
            if(way < best_way)
            {
                // Are we at that flag or is there a path to it?
                if(way == 0 || gwg->FindHumanPath(pos, (*it)->GetPos(), wander_radius, false, &way) != 0xFF)
                {
                    // gucken, ob ein Weg zu einem Warenhaus führt
                    if(gwg->GetPlayer(player).FindWarehouse(**it, FW::Condition_StoreFigure, 0, true, &job_, false))
                    {
                        // dann nehmen wir die doch glatt
                        best_way = way;
                        best_flag = *it;
                        if(way == 0)
                            break; // Can't get better
                    }
                }
                else if(burned_wh_id != 0xFFFFFFFF)
                {
                    // Flagge nicht möglich zugänglich bei einem Flüchting aus einem abgebrannten Lagerhaus?
                    // --> der ganzen Gruppe Bescheid sagen, damit die nicht auch alle sinnlos einen Weg zu
                    // dieser Flagge suchen

                    //TODO: Actually it is possible! E.g. between us and the flag is a river, so we won't find a path within the radius
                    // but others (on the other side) could --> Remove ImpossibleForBWU?
                    (*it)->ImpossibleForBWU(burned_wh_id);
                }
            }
        }

        if(best_flag)
        {
            // bestmögliche schließlich nehmen
            wander_way = 0xFFFF;
            flagPos_ = best_flag->GetPos();
            flag_obj_id = best_flag->GetObjId();
            WanderToFlag();
            return;
        }

        // Wurde keine Flagge gefunden?

        // Haben wir noch Versuche?
        if(--wander_tryings > 0)
        {
            // von vorne beginnen wieder mit Rumirren
            wander_way = WANDER_WAY_MIN + RANDOM.Rand(__FILE__, __LINE__, GetObjId(), WANDER_WAY_MAX - WANDER_WAY_MIN);
        }
        else
        {
            // Genug rumgeirrt, wir finden halt einfach nichts --> Sterben
            Die();
            return;
        }
    }

    // weiter umherirren, einfach in eine zufällige Richtung
    // Müssen dabei natürlich aufpassen, dass wir nur dorthin gehen wo es auch für Figuren möglich ist
    unsigned char doffset = RANDOM.Rand(__FILE__, __LINE__, GetObjId(), 6);
    for(unsigned char d = 0; d < 6; ++d)
    {
        unsigned char dir = (d + doffset) % 6;

        if(gwg->IsNodeForFigures(gwg->GetNeighbour(pos, dir)) && gwg->IsNodeToNodeForFigure(pos, dir))
        {
            StartWalking(dir);
            --wander_way;
            return;
        }
    }

    // Wir sind eingesperrt! Kein Weg mehr gefunden --> Sterben
    Die();
}

void noFigure::WanderFailedTrade()
{
    DieFailedTrade();
    return;
}

void noFigure::WanderToFlag()
{
    // Existiert die Flagge überhaupt noch?
    noBase* no = gwg->GetNO(flagPos_);
    if(no->GetObjId() != flag_obj_id)
    {
        // Wenn nicht, wieder normal weiter rumirren
        StartWandering();
        Wander();
        return;
    }

    // Sind wir schon da?
    if(pos == flagPos_)
    {
        // Gibts noch nen Weg zu einem Lagerhaus?
        assert(gwg->GetSpecObj<noRoadNode>(pos));
        if(nobBaseWarehouse* wh = gwg->GetPlayer(player).FindWarehouse(
                                      *gwg->GetSpecObj<noRoadNode>(pos), FW::Condition_StoreFigure, 0, true, &job_, false))
        {
            // ja, dann können wir ja hingehen
            goal_ = wh;
            cur_rs = NULL;
            rs_pos = 0;
            fs = FS_GOHOME;
            wh->AddDependentFigure(this);
            WalkToGoal();
            return;
        }
        else
        {
            // Wenn nicht, wieder normal weiter rumirren
            StartWandering();
            Wander();
            return;
        }
    }

    // Weiter zur Flagge gehen
    // Gibts noch nen Weg dahin bzw. existiert die Flagge noch?
    unsigned char dir = gwg->FindHumanPath(pos, flagPos_, 60, false);
    if(dir != 0xFF)
    {
        // weiter hinlaufen
        StartWalking(dir);
    }
    else
    {
        // Wenn nicht, wieder normal weiter rumirren
        StartWandering();
        Wander();
    }
}

void noFigure::CorrectSplitData(const RoadSegment* const rs2)
{
    // cur_rs entspricht Teilstück 1 !

    // Wenn man sich auf den ersten Teilstück befindet...
    if((rs_pos < cur_rs->GetLength() && !rs_dir) || (rs_pos > rs2->GetLength() && rs_dir))
    {
        // Nur Position berichtigen
        if(rs_dir)
            rs_pos -= rs2->GetLength();
    }

    // Wenn man auf dem 2. steht, ...
    else if((rs_pos > cur_rs->GetLength() && !rs_dir) || (rs_pos < rs2->GetLength() && rs_dir))
    {
        // Position berichtigen (wenn man in umgekehrter Richtung läuft, beibehalten!)
        if(!rs_dir)
            rs_pos -= cur_rs->GetLength();

        // wir laufen auf dem 2. Teilstück
        cur_rs = rs2;
    }
    else if((rs_pos == cur_rs->GetLength() && !rs_dir) || (rs_pos == rs2->GetLength() && rs_dir))
    {
        // wir stehen genau in der Mitte
        // abhängig von der Richtung machen, in die man gerade läuft
        if(GetCurMoveDir() == rs2->GetRoute(0))
        {
            // wir laufen auf dem 2. Teilstück
            cur_rs = rs2;
            // und wir sind da noch am Anfang
            rs_pos = 0;
        }
        else if(GetCurMoveDir() == (cur_rs->GetRoute(cur_rs->GetLength() - 1) + 3) % 6)
        {
            // wir laufen auf dem 1. Teilstück

            // und wir sind da noch am Anfang
            rs_pos = 0;
        }
        else
        {
            // Wahrscheinlich stehen wir
            // dann einfach auf das 2. gehen
            cur_rs = rs2;
            rs_pos = 0;
            rs_dir = 0;
        }
    }

    CorrectSplitData_Derived();
}

/// Wird aufgerufen, wenn die Straße unter der Figur geteilt wurde (für abgeleitete Klassen)
void noFigure::CorrectSplitData_Derived()
{
}

void noFigure::DrawWalkingBobCarrier(int x, int y, unsigned int ware, bool fat)
{
    // Wenn wir warten auf ein freies Plätzchen, müssen wir den stehend zeichnen!
    unsigned ani_step = waiting_for_free_node ? 2 : GAMECLIENT.Interpolate(ASCENT_ANIMATION_STEPS[ascent], current_ev) % 8;

    // Wenn man wartet, stehend zeichnen, es sei denn man wartet mittem auf dem Weg!
    Point<int> realPos = Point<int>(x, y);
    if(!waiting_for_free_node || pause_walked_gf)
        realPos += CalcFigurRelative();

    LOADER.carrier_cache[ware][GetCurMoveDir()][ani_step][fat].draw(realPos.x, realPos.y, COLOR_WHITE, COLORS[gwg->GetPlayer(player).color]);
}



void noFigure::DrawWalkingBobJobs(int x, int y, unsigned int job)
{
    if ((job == JOB_SCOUT) || ((job >= JOB_PRIVATE) && (job <= JOB_GENERAL)))
    {
        DrawWalking(x, y, LOADER.GetBobN("jobs"), JOB_CONSTS[job].jobs_bob_id + NATION_RTTR_TO_S2[gwg->GetPlayer(player).nation] * 6, false);
        return;
    }

    // Wenn wir warten auf ein freies Plätzchen, müssen wir den stehend zeichnen!
    unsigned ani_step = waiting_for_free_node ? 2 : GAMECLIENT.Interpolate(ASCENT_ANIMATION_STEPS[ascent], current_ev) % 8;

    // Wenn man wartet, stehend zeichnen, es sei denn man wartet mittem auf dem Weg!
    Point<int> realPos = Point<int>(x, y);
    if(!waiting_for_free_node || pause_walked_gf)
        realPos += CalcFigurRelative();

    LOADER.bob_jobs_cache[gwg->GetPlayer(player).nation][job][GetCurMoveDir()][ani_step].draw(realPos.x, realPos.y, 0xFFFFFFFF, COLORS[gwg->GetPlayer(player).color]);
}


void noFigure::DrawWalking(int x, int y, glArchivItem_Bob* file, unsigned int id, bool fat, bool waitingsoldier)
{
    // Wenn wir warten auf ein freies Plätzchen, müssen wir den stehend zeichnen!
    unsigned ani_step = waiting_for_free_node || waitingsoldier ? 2 : GAMECLIENT.Interpolate(ASCENT_ANIMATION_STEPS[ascent], current_ev) % 8;

    // Wenn man wartet, stehend zeichnen, es sei denn man wartet mittem auf dem Weg!
    Point<int> realPos = Point<int>(x, y);
    if(!waitingsoldier && (!waiting_for_free_node || pause_walked_gf))
        realPos += CalcFigurRelative();
    if(file)
        file->Draw(id, GetCurMoveDir(), fat, ani_step, realPos.x, realPos.y, COLORS[gwg->GetPlayer(player).color]);
    DrawShadow(realPos.x, realPos.y, ani_step, GetCurMoveDir());
}

/// Zeichnet standardmäßig die Figur, wenn sie läuft aus einem bestimmten normalen LST Archiv
void noFigure::DrawWalking(int x, int y, const char* const file, unsigned int id)
{
    // Wenn wir warten, ani-step 2 benutzen
    unsigned ani_step = waiting_for_free_node ? 2 : GAMECLIENT.Interpolate(ASCENT_ANIMATION_STEPS[ascent], current_ev) % 8;

    // Wenn man wartet, stehend zeichnen, es sei denn man wartet mittem auf dem Weg!
    Point<int> realPos = Point<int>(x, y);
    if(!waiting_for_free_node || pause_walked_gf)
        realPos += CalcFigurRelative();

    LOADER.GetImageN(file, id + ((GetCurMoveDir() + 3) % 6) * 8 + ani_step)->Draw(realPos.x, realPos.y, 0, 0, 0, 0, 0, 0, COLOR_WHITE, COLORS[gwg->GetPlayer(player).color]);
    DrawShadow(realPos.x, realPos.y, ani_step, GetCurMoveDir());
}

void noFigure::DrawWalking(int x, int y)
{
    // Figurentyp unterscheiden
    switch(job_)
    {
        case JOB_PACKDONKEY:
        {
            // Wenn wir warten, ani-step 2 benutzen
            unsigned ani_step = waiting_for_free_node ? 2 : GAMECLIENT.Interpolate(ASCENT_ANIMATION_STEPS[ascent], current_ev) % 8;

            // Wenn man wartet, stehend zeichnen, es sei denn man wartet mittem auf dem Weg!
            Point<int> realPos = Point<int>(x, y);
            if(!waiting_for_free_node || pause_walked_gf)
                realPos += CalcFigurRelative();

            // Esel
            LOADER.GetMapImageN(2000 + ((GetCurMoveDir() + 3) % 6) * 8 + ani_step)->Draw(realPos.x, realPos.y);
            // Schatten des Esels
            LOADER.GetMapImageN(2048 + GetCurMoveDir() % 3)->Draw(realPos.x, realPos.y, 0, 0, 0, 0, 0, 0, COLOR_SHADOW);
        } return;
        case JOB_CHARBURNER:
        {
            DrawWalking(x, y, "charburner_bobs", 53);
        } return;
        default:
        {
            DrawWalkingBobJobs(x, y, job_);
        } return;
    }


}

void noFigure::Die()
{
    // Weg mit mir
    gwg->RemoveFigure(this, pos);
    em->AddToKillList(this);
    // ggf. Leiche hinlegen, falls da nix ist
    if(!gwg->GetSpecObj<noBase>(pos))
        gwg->SetNO(new noSkeleton(pos), pos);

    RemoveFromInventory();

    // Sichtbarkeiten neu berechnen für Erkunder und Soldaten
    CalcVisibilities(pos);
}


void noFigure::RemoveFromInventory()
{
    // Wars ein Bootmann? Dann Boot und Träger abziehen
    if(job_ == JOB_BOATCARRIER)
    {
        gwg->GetPlayer(player).DecreaseInventoryJob(JOB_HELPER, 1);
        gwg->GetPlayer(player).DecreaseInventoryWare(GD_BOAT, 1);
    }
    else
        gwg->GetPlayer(player).DecreaseInventoryJob(job_, 1);
}

void noFigure::DieFailedTrade()
{
    // Weg mit mir
    gwg->RemoveFigure(this, pos);
    em->AddToKillList(this);
    // ggf. Leiche hinlegen, falls da nix ist
    if(!gwg->GetSpecObj<noBase>(pos))
        gwg->SetNO(new noSkeleton(pos), pos);
    // Sichtbarkeiten neu berechnen für Erkunder und Soldaten
    //CalcVisibilities(x,y);
}

void noFigure::NodeFreed(const MapPoint pt)
{
    // Stehen wir gerade aus diesem Grund?
    if(waiting_for_free_node)
    {
        // Ist das der Punkt, zu dem wir hin wollen?
        if(pt == gwg->GetNeighbour(this->pos, GetCurMoveDir()))
        {
            // Gehen wir in ein Gebäude? Dann wieder ausgleichen, weil wir die Türen sonst doppelt aufmachen!
            if(GetCurMoveDir() == 1 && gwg->GetNO(gwg->GetNeighbour(this->pos, 1))->GetType() == NOP_BUILDING)
                gwg->GetSpecObj<noBuilding>(gwg->GetNeighbour(this->pos, 1))->CloseDoor();
            // oder aus einem raus?
            if(GetCurMoveDir() == 4 && gwg->GetNO(this->pos)->GetType() == NOP_BUILDING)
                gwg->GetSpecObj<noBuilding>(this->pos)->CloseDoor();

            // Wir stehen nun nicht mehr
            waiting_for_free_node = false;

            // Dann loslaufen
            StartWalking(GetCurMoveDir());

            // anderen Leuten noch ggf Bescheid sagen
            gwg->RoadNodeAvailable(this->pos);
        }
    }
}

void noFigure::Abrogate()
{
    // Arbeisplatz oder Laghaus Bescheid sagen
    if(fs == FS_GOHOME)
    {
        //goal might by NULL if goal was a harbor that got destroyed during sea travel
        if(goal_)
        {
            assert(dynamic_cast<nobBaseWarehouse*>(goal_));
            static_cast<nobBaseWarehouse*>(goal_)->RemoveDependentFigure(this);
            goal_ = NULL;
        }else
        {
            if(!on_ship) //no goal but going home - should not happen
            {
                LOG.lprintf("noFigure::Abrogate - GOHOME figure has no goal and is not on a ship - player %i state %i pos %u,%u \n", player, fs, pos.x, pos.y);
                //assert(false);
            }
        }
    }
    else
    {
        goal_ = NULL;
        AbrogateWorkplace();
    }
}


void noFigure::StopIfNecessary(const MapPoint pt)
{
    // Lauf ich auf Wegen --> wenn man zum Ziel oder Weg läuft oder die Träger, die natürlich auch auf Wegen arbeiten
    if(fs == FS_GOHOME || fs == FS_GOTOGOAL || (fs == FS_JOB && GetGOT() == GOT_NOF_CARRIER))
    {
        // Laufe ich zu diesem Punkt?
        if(current_ev)
        {
            if(!waiting_for_free_node && gwg->GetNeighbour(this->pos, GetCurMoveDir()) == pt)
            {
                // Dann stehenbleiben
                PauseWalking();
                waiting_for_free_node = true;
                gwg->StopOnRoads(this->pos, GetCurMoveDir());
            }
        }
    }
}


/// Sichtbarkeiten berechnen für Figuren mit Sichtradius (Soldaten, Erkunder) vor dem Laufen
void noFigure::CalcVisibilities(const MapPoint pt)
{
    // Sichtbarkeiten neu berechnen für Erkunder und Soldaten
    if(GetVisualRange())
        // An alter Position neu berechnen
        gwg->RecalcVisibilitiesAroundPoint(pt, GetVisualRange(), player, NULL);
}

/// Informiert die Figur, dass für sie eine Schiffsreise beginnt
void noFigure::StartShipJourney()
{
    // We should not be in the world, as we start the journey from a harbor -> We are in that harbor
    assert(!helpers::contains(gwg->GetFigures(pos), this));

    pos = MapPoint::Invalid();
    on_ship = true;
}

void noFigure::ArrivedByShip(const MapPoint harborPos)
{
    assert(on_ship);
    pos = harborPos;
}

/// Informiert die Figur, wenn Kreuzfahrt beendet ist
void noFigure::ShipJourneyEnded()
{
    on_ship = false;
}

/// Examines the route (maybe harbor, road destroyed?) before start shipping
MapPoint noFigure::ExamineRouteBeforeShipping(unsigned char& newDir)
{
    MapPoint next_harbor;
    // Calc new route
    const noRoadNode* roadNode = gwg->GetSpecObj<noRoadNode>(pos);
    if(!roadNode || !goal_)
        newDir = INVALID_DIR;
    else
        newDir = gwg->FindHumanPathOnRoads(*roadNode, *goal_, NULL, &next_harbor);

    if(newDir == 0xff)
        Abrogate();

    // Going by ship?
    if(newDir == SHIP_DIR)
        // All ok, return next harbor (could be another one!)
        return next_harbor;
    else
        return MapPoint(0, 0);
}

///////////////////////////////////////////////////////////////////////////////
// EOF
///////////////////////////////////////////////////////////////////////////////

