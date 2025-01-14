// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "Game.h"
#include "PlayerInfo.h"
#include "network/GameClient.h"
#include "ogl/glAllocator.h"
#include "world/MapLoader.h"
#include "libsiedler2/libsiedler2.h"
#include <rttr/test/Fixture.hpp>
#include <benchmark/benchmark.h>
#include <array>
#include <test/testConfig.h>
#include <utility>

constexpr std::array<std::tuple<const char*, MapPoint, MapPoint>, 7> routes = {{{"Simple 1", {85, 147}, {87, 150}},
                                                                                {"Simple 2", {85, 147}, {85, 152}},
                                                                                {"Simple 3", {85, 147}, {79, 149}},
                                                                                {"Medium 1", {85, 147}, {77, 163}},
                                                                                {"Medium 2", {85, 147}, {79, 127}},
                                                                                {"Hard", {21, 200}, {42, 188}},
                                                                                {"Water", {152, 66}, {198, 34}}}};

static void BM_World(benchmark::State& state)
{
    rttr::test::Fixture f;
    libsiedler2::setAllocator(new GlAllocator);

    std::vector<PlayerInfo> players(2);
    for(auto& player : players)
        player.ps = PlayerState::Occupied;
    auto game = std::make_shared<Game>(GlobalGameSettings(), 0, players);
    GameWorld& world = game->world_;
    MapLoader loader(world);
    if(!loader.Load(rttr::test::rttrBaseDir / "data/RTTR/MAPS/NEW/AM_FANGDERZEIT.SWD"))
        state.SkipWithError("Map failed to load");

    const auto& curValues = routes[static_cast<size_t>(state.range())];
    state.SetLabel(std::get<0>(curValues));
    const MapPoint start = std::get<1>(curValues);
    const MapPoint goal = std::get<2>(curValues);

    for(auto _ : state)
    {
        const bool result = state.range() < 6 ? world.FindHumanPath(start, goal).has_value() :
                                                world.FindShipPath(start, goal, 600, nullptr, nullptr);
        benchmark::DoNotOptimize(result);
    }
}
BENCHMARK(BM_World)->DenseRange(0, routes.size() - 1);
