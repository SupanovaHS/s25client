// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "figures/noFigure.h"
#include "gameTypes/GoodTypes.h"

class nobUsual;
class nobBaseWarehouse;
class SerializedGameData;

/// Repräsentiert einen Arbeiter in einem Gebäude
class nofBuildingWorker : public noFigure
{
public:
    /// Was der gerade so schönes macht
    enum class State : uint8_t
    {
        FigureWork,                         /// Work of the noFigure (running to work, wandering around, etc.)
        EnterBuilding,                      /// Entering the building
        Waiting1,                           /// Waiting until you start producing
        Waiting2,                           /// Waiting after producing until goods are taken out (craftsmen only)
        CarryoutWare,                       /// Carrying out the goods
        Work,                               /// work
        WaitingForWaresOrProductionStopped, /// Waiting for goods or because production was stopped
        WalkToWorkpoint,                    /// Walk to "work point" (farm workers only)
        WalkingHome,                        /// walk back home from the work point (farm workers only)
        WaitForWareSpace,                   /// wait for a free space on the flag in front of the building
        HunterChasing,                      /// Hunter: pursues the animal up to a certain distance
        HunterFindingShootingpoint,  /// Hunter: searches for a point around the animal from which he can shoot it
        HunterShooting,              /// Jäger: Shoot an animal
        HunterWalkingToCadaver,      /// Hunter: Run to the carcass
        HunterEviscerating,          /// Hunter: eviscerate animal
        CatapultTargetBuilding,      /// Catapult: Turns the catapult up towards the target and shoots
        CatapultBackoff,             /// Katapult: stop shooting and turn the catapult back to its starting position
        HunterWaitingForAnimalReady, /// Hunter: Arrived at shooting pos and waiting for animal to be ready to

    };
    friend constexpr auto maxEnumValue(State) { return State::HunterWaitingForAnimalReady; }

protected:
    State state;

    /// Arbeitsplatz (Haus, in dem er arbeitet)
    nobUsual* workplace;

    // Ware, die er evtl gerade trägt
    helpers::OptionalEnum<GoodType> ware;

    /// Hat der Bauarbeiter bei seiner Arbeit Sounds von sich gegeben (zu Optimeriungszwecken)
    bool was_sounding;

    /// wird von abgeleiteten Klassen aufgerufen, wenn sie die Ware an der Fahne vorm Gebäude ablegen wollen (oder auch
    /// nicht) also fertig mit Arbeiten sind
    void WorkingReady();
    /// wenn man beim Arbeitsplatz "kündigen" soll, man das Laufen zum Ziel unterbrechen muss (warum auch immer)
    void AbrogateWorkplace() override;
    /// Tries to start working.
    /// Checks preconditions (production enabled, wares available...) and starts the pre-Work-Waiting period if ok
    void TryToWork();
    /// Returns true, when there are enough wares available for working.
    /// Note: On false, we will wait for the next ware or production change till checking again
    virtual bool AreWaresAvailable() const;

private:
    /// von noFigure aufgerufen
    void Walked() override;      // wenn man gelaufen ist
    void GoalReached() override; // wenn das Ziel erreicht wurde

protected:
    /// Malt den Arbeiter beim Arbeiten
    virtual void DrawWorking(DrawPoint drawPt) = 0;
    static constexpr unsigned short CARRY_ID_CARRIER_OFFSET = 100;
    /// Ask derived class for an ID into JOBS.BOB when the figure is carrying a ware
    /// Use GD_* + CARRY_ID_CARRIER_OFFSET for using carrier graphics
    virtual unsigned short GetCarryID() const = 0;
    /// Laufen an abgeleitete Klassen weiterleiten
    virtual void WalkedDerived() = 0;
    /// Arbeit musste wegen Arbeitsplatzverlust abgebrochen werden
    virtual void WorkAborted();
    /// Arbeitsplatz wurde erreicht
    virtual void WorkplaceReached();

    /// Draws the figure while returning home / entering the building (often carrying wares)
    virtual void DrawWalkingWithWare(DrawPoint drawPt);
    /// Zeichnen der Figur in sonstigen Arbeitslagen
    virtual void DrawOtherStates(DrawPoint drawPt);

public:
    State GetState() const { return state; }

    nofBuildingWorker(Job job, MapPoint pos, unsigned char player, nobUsual* workplace);
    nofBuildingWorker(Job job, MapPoint pos, unsigned char player, nobBaseWarehouse* goalWh);
    nofBuildingWorker(SerializedGameData& sgd, unsigned obj_id);

    void Destroy() override
    {
        RTTR_Assert(!workplace);
        noFigure::Destroy();
    }
    void Serialize(SerializedGameData& sgd) const override;

    void Draw(DrawPoint drawPt) override;

    /// Wenn eine neue Ware kommt oder die Produktion wieder erlaubt wurde, wird das aufgerufen
    void GotWareOrProductionAllowed();
    /// Wenn wieder Platz an der Flagge ist und eine Ware wieder rausgetragen werden kann
    bool FreePlaceAtFlag();
    /// Wenn das Haus des Arbeiters abbrennt
    void LostWork();
    /// Wird aufgerufen, nachdem die Produktion in dem Gebäude, wo er arbeitet, verboten wurde
    void ProductionStopped();
};
