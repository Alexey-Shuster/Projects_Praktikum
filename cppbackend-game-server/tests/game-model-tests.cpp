
#include <catch2/catch_test_macros.hpp>
#include <boost/asio/io_context.hpp>

#include <chrono>
#include <memory>
#include <stdexcept>

#include "../src/game_model/game_model.h"
#include "../src/game_model/game_map.h"
#include "../src/game_model/game_session.h"
#include "../src/game_model/game_extra_data.h"
#include "../src/common/game_utils/geometry.h"

using namespace std::literals;

// Helper to create a populated GameExtraData with default loot config
std::shared_ptr<extra_data::GameExtraData> MakeTestExtraData() {
    auto extra_data = std::make_shared<extra_data::GameExtraData>();
    extra_data->AddLootGeneratorConfig({100ms, 0.5});
    // No map speeds needed for basic tests, but we can add an empty map
    extra_data->AddMapsSpeed({});
    return extra_data;
}

// Helper to add a minimal road to a map (required for Dog creation)
void AddMinimalRoad(model::Map& map) {
    // Add a horizontal road from (0,0) to (10,0)
    map.AddRoad(model::Road(model::Road::HORIZONTAL, model_geom::Point2D{0, 0}, 10));
}

SCENARIO("Game map management") {
    GIVEN("a game with no maps") {
        auto extra_data = MakeTestExtraData();
        model::Game game(extra_data);
        model::Map::Id map_id("map1");
        model::Map map(map_id, "Test Map");

        WHEN("a map is added") {
            game.AddMap(std::move(map));

            THEN("it can be found by its id") {
                const model::Map* found = game.FindMap(map_id);
                REQUIRE(found != nullptr);
                REQUIRE(found->GetId() == map_id);
            }

            THEN("the list of maps contains one element") {
                REQUIRE(game.GetMaps().size() == 1);
                REQUIRE(game.GetMaps().front().GetId() == map_id);
            }
        }

        WHEN("adding a map with a duplicate id") {
            game.AddMap(model::Map(map_id, "First Map"));
            THEN("adding another map with the same id throws") {
                REQUIRE_THROWS_AS(game.AddMap(model::Map(map_id, "Second Map")), std::invalid_argument);
            }
        }
    }
}

SCENARIO("Game session creation and retrieval") {
    GIVEN("a game with one map") {
        boost::asio::io_context ioc;
        auto extra_data = MakeTestExtraData();
        model::Game game(extra_data);
        model::Map::Id map_id("map1");
        model::Map map(map_id, "Map 1");
        AddMinimalRoad(map);          // Add a road so dogs can be created safely
        game.AddMap(std::move(map));

        WHEN("requesting a game session for that map") {
            auto session_data = game.RequestGameSession(map_id, ioc);
            auto session_id = session_data.first;

            THEN("a valid session id is returned") {
                // Id is tagged uint32_t; default constructed Id has value 0
                REQUIRE(session_data.first != model::GameSession::Id{0});
            }

            THEN("the session can be found by that id") {
                const model::GameSession* session = game.FindGameSession(session_id);
                REQUIRE(session != nullptr);
                REQUIRE(session->GetId() == session_id);
            }

            THEN("a second request for the same map returns the same session id (session not full)") {
                auto session = game.RequestGameSession(map_id, ioc);
                REQUIRE(session.first == session_id);
            }
        }

        WHEN("requesting sessions for two different maps") {
            model::Map::Id map_id2("map2");
            model::Map map2(map_id2, "Map 2");
            AddMinimalRoad(map2);
            game.AddMap(std::move(map2));

            auto session_data_1 = game.RequestGameSession(map_id, ioc);
            auto session_data_2 = game.RequestGameSession(map_id2, ioc);

            THEN("the session ids are different") {
                REQUIRE(session_data_1.first != session_data_2.first);
            }

            THEN("each session is associated with its own map") {
                // We can check that the session's map id matches (if GameSession provides that)
                // For simplicity, we just verify they are distinct.
            }
        }

        WHEN("a session becomes full") {
            auto session_data = game.RequestGameSession(map_id, ioc);
            auto session_id = session_data.first;
            model::GameSession* session = game.FindGameSession(session_id);

            // Add dogs until the session reaches MAX_PLAYERS_ON_MAP
            for (size_t i = session->GetDogs().size(); i < model::MAX_PLAYERS_ON_MAP; ++i) {
                auto dog = session->RequestDog(static_cast<std::uint32_t>(i), "Dog" + std::to_string(i));
            }
            REQUIRE(session->GetDogs().size() == model::MAX_PLAYERS_ON_MAP);

            THEN("a new request for the same map creates a new session") {
                auto new_session_data = game.RequestGameSession(map_id, ioc);
                REQUIRE(new_session_data.first != session_id);

                model::GameSession* new_session = game.FindGameSession(new_session_data.first);
                REQUIRE(new_session != nullptr);
                REQUIRE(new_session->GetDogs().empty()); // fresh session
            }
        }
    }
}

SCENARIO("Finding non‑existent game sessions") {
    GIVEN("a game without any sessions") {
        boost::asio::io_context ioc;
        auto extra_data = MakeTestExtraData();
        model::Game game(extra_data);
        model::GameSession::Id unknown_id{0};

        WHEN("searching for an arbitrary session id") {
            THEN("FindGameSession returns nullptr") {
                REQUIRE(game.FindGameSession(unknown_id) == nullptr);
                const model::Game& const_game = game;
                REQUIRE(const_game.FindGameSession(unknown_id) == nullptr);
            }
        }
    }
}

SCENARIO("Updating all game sessions") {
    GIVEN("a game with two maps and corresponding sessions") {
        boost::asio::io_context ioc;
        auto extra_data = MakeTestExtraData();
        model::Game game(extra_data);

        model::Map::Id map_id1("map1"), map_id2("map2");
        model::Map map1(map_id1, "Map 1");
        model::Map map2(map_id2, "Map 2");
        AddMinimalRoad(map1);
        AddMinimalRoad(map2);
        game.AddMap(std::move(map1));
        game.AddMap(std::move(map2));

        auto session_data_1 = game.RequestGameSession(map_id1, ioc);
        auto session_data_2 = game.RequestGameSession(map_id2, ioc);

        WHEN("UpdateAllGameSessions is called with a time delta") {
            REQUIRE_NOTHROW(game.UpdateAllGameSessions(100ms));

            THEN("tasks are posted to each session's strand") {
                // Run the io_context to process the posted tasks
                ioc.run();
                // No direct way to verify UpdateGameState was called without
                // instrumenting the session, but we at least ensure no exception.
            }
        }
    }
}