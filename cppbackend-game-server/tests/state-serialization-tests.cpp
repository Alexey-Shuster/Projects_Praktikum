#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <catch2/catch_test_macros.hpp>
#include <sstream>
#include <memory>

#include "../src/game_model/game_model.h"
#include "../src/game_app/players.h"
#include "../src/game_repr/game_repr.h"

namespace {
using namespace std::literals;

using InputArchive = boost::archive::text_iarchive;
using OutputArchive = boost::archive::text_oarchive;

// Fixture provides only the stream and output archive.
// Input archive must be created after writing, with stream rewound.
struct Fixture {
    std::stringstream strm;
    OutputArchive output_archive{strm};
};

// Helper to create a simple map with one road
std::shared_ptr<model::Map> CreateTestMap(const model::Map::Id& id = model::Map::Id("map1"s)) {
    auto map = std::make_shared<model::Map>(id, "Test Map"s);
    map.get()->AddRoad(model::Road(model::Road::HORIZONTAL, {0, 0}, 10));
    map.get()->SetDefaultSpeed({1.0, 1.0});
    return map;
}

// Helper to create game extra data with one loot type for the given map
std::shared_ptr<extra_data::GameExtraData> CreateExtraDataWithLoot(const model::Map::Id& map_id) {
    auto extra = std::make_shared<extra_data::GameExtraData>();
    extra_data::LootData loot_data;
    loot_data.name = "coin";
    loot_data.value = 1;
    loot_data.scale = 0.5;
    std::vector<extra_data::LootData> loot_types{std::move(loot_data)};
    extra->AddLootTypes(map_id, std::move(loot_types));
    return extra;
}

// Helper to create a loot generator
std::shared_ptr<loot_gen::LootGenerator> CreateLootGenerator() {
    return std::make_shared<loot_gen::LootGenerator>(1s, 0.5);
}

} // namespace

//------------------------------------------------------------------------------
// Point serialization (simple test)
//------------------------------------------------------------------------------
SCENARIO_METHOD(Fixture, "Point serialization") {
    GIVEN("A point") {
        const model_geom::Point2D p{10, 20};
        WHEN("point is serialized") {
            output_archive << p.x << p.y;

            THEN("it is equal to point after serialization") {
                strm.seekg(0);
                InputArchive input_archive{strm};
                model_geom::Point2D restored_point;
                input_archive >> restored_point.x >> restored_point.y;
                CHECK(p == restored_point);
            }
        }
    }
}

//------------------------------------------------------------------------------
// DogRepr – minimal test (no bag, no road)
//------------------------------------------------------------------------------
SCENARIO_METHOD(Fixture, "DogRepr basic serialization") {
    GIVEN("a dog with basic fields") {
        auto map = CreateTestMap();
        model::Dog original(42, "Pluto"s, map.get());
        original.SetPosition({10.5, 20.5});
        original.SetSpeed({2.5, -1.5});
        original.SetDirection(app_geom::Direction2D::UP);
        original.AddScore(100);

        WHEN("the dog is serialized via DogRepr") {
            serialize_game_save::DogRepr repr(original);
            output_archive << repr;

            THEN("it can be deserialized and restored to an equivalent dog") {
                strm.seekg(0);
                InputArchive input_archive{strm};
                serialize_game_save::DogRepr restored_repr;
                input_archive >> restored_repr;

                // No loot in bag, so id_to_loot map is empty
                std::unordered_map<loot::LootObjectId, loot::LootObject*> id_to_loot;
                auto restored = restored_repr.Restore(map.get(), id_to_loot);

                CHECK(original.GetId() == restored.GetId());
                CHECK(original.GetName() == restored.GetName());
                CHECK(original.GetPosition() == restored.GetPosition());
                CHECK(original.GetSpeed() == restored.GetSpeed());
                CHECK(original.GetDirection() == restored.GetDirection());
                CHECK(original.GetScore() == restored.GetScore());
                CHECK(restored.GetBag().empty());
                // Current road is not set, so it should be the zero road
                CHECK(restored.GetCurrentRoad() == &map->GetZeroRoad());
            }
        }
    }
}

//------------------------------------------------------------------------------
// LootStorageRepr – minimal test (empty storage)
//------------------------------------------------------------------------------
SCENARIO_METHOD(Fixture, "LootStorageRepr empty serialization") {
    GIVEN("an empty loot storage") {
        auto map_id = model::Map::Id("map1"s);
        auto extra = CreateExtraDataWithLoot(map_id);
        loot::LootStorage original;
        original.SetNextId(42);

        WHEN("the storage is serialized via LootStorageRepr") {
            serialize_game_save::LootStorageRepr repr(original, map_id, extra);
            output_archive << repr;

            THEN("it can be deserialized and restored to an equivalent storage") {
                strm.seekg(0);
                InputArchive input_archive{strm};
                serialize_game_save::LootStorageRepr restored_repr;
                input_archive >> restored_repr;

                loot::LootStorage restored;
                std::unordered_map<loot::LootObjectId, loot::LootObject*> id_to_loot;
                const auto& loot_types = extra->GetLootTypes(map_id);
                restored_repr.Restore(restored, loot_types, id_to_loot);

                CHECK(original.GetNextId() == restored.GetNextId());
                CHECK(restored.GetLootObjects().empty());
                CHECK(id_to_loot.empty());
            }
        }
    }
}

//------------------------------------------------------------------------------
// GameSessionRepr – minimal test (empty session)
//------------------------------------------------------------------------------
SCENARIO_METHOD(Fixture, "GameSessionRepr empty serialization") {
    GIVEN("an empty game session") {
        boost::asio::io_context ioc;
        auto map = CreateTestMap();
        auto extra = CreateExtraDataWithLoot(map->GetId());
        auto loot_gen = CreateLootGenerator();

        model::GameSession::Id session_id(1u);
        model::GameSession session(session_id, "EmptySession"s, map.get(), ioc, loot_gen, extra);

        WHEN("the session is serialized via GameSessionRepr") {
            serialize_game_save::GameSessionRepr repr(session);
            output_archive << repr;

            THEN("it can be deserialized and restored into a new game") {
                strm.seekg(0);
                InputArchive input_archive{strm};
                serialize_game_save::GameSessionRepr restored_repr;
                input_archive >> restored_repr;

                // Create a game with the same map
                model::Game game(extra);
                game.AddMap(*map);
                std::unordered_map<model::GameSession::Id, model::GameSession*, model::Game::GameSessionIdHasher> sessions_by_id;

                restored_repr.Restore(game, ioc, loot_gen, extra, sessions_by_id);

                auto* restored_session = game.FindGameSession(session_id);
                REQUIRE(restored_session != nullptr);
                CHECK(restored_session->GetId() == session_id);
                CHECK(restored_session->GetName() == "EmptySession");
                CHECK(restored_session->GetMap()->GetId() == map->GetId());
                CHECK(restored_session->GetDogs().empty());
                CHECK(restored_session->GetLootStorage().GetLootObjects().empty());
            }
        }
    }
}

//------------------------------------------------------------------------------
// PlayersRepr – minimal test (no players)
//------------------------------------------------------------------------------
SCENARIO_METHOD(Fixture, "PlayersRepr empty serialization") {
    GIVEN("an empty Players container") {
        boost::asio::io_context ioc;
        auto map = CreateTestMap();
        auto extra = CreateExtraDataWithLoot(map->GetId());
        auto loot_gen = CreateLootGenerator();

        model::Game game(extra);
        game.AddMap(*map);
        model::GameSession::Id session_id(1u);
        model::GameSession session(session_id, "TestSession"s, map.get(), ioc, loot_gen, extra);
        game.AddRestoredSession(std::move(session));

        app::Players original_players(game);  // empty initially

        WHEN("the players are serialized via PlayersRepr") {
            serialize_game_save::PlayersRepr repr(original_players);
            output_archive << repr;

            THEN("they can be deserialized and restored into a new Players container") {
                strm.seekg(0);
                InputArchive input_archive{strm};
                serialize_game_save::PlayersRepr restored_repr;
                input_archive >> restored_repr;

                // Create a new game with the same session
                model::Game restored_game(extra);
                restored_game.AddMap(*map);
                model::GameSession restored_session(session_id, "TestSession"s, map.get(), ioc, loot_gen, extra);
                restored_game.AddRestoredSession(std::move(restored_session));
                auto* session_ptr = restored_game.FindGameSession(session_id);

                app::Players restored_players(restored_game);
                std::unordered_map<model::GameSession::Id, model::GameSession*, model::Game::GameSessionIdHasher> sessions_by_id;
                sessions_by_id[session_id] = session_ptr;

                restored_repr.Restore(restored_players, sessions_by_id);

                CHECK(original_players.GetNextPlayerId() == restored_players.GetNextPlayerId());
                CHECK(restored_players.GetAllPlayers().empty());
            }
        }
    }
}

//------------------------------------------------------------------------------
// GameRepr – minimal test (no sessions, no players)
//------------------------------------------------------------------------------
SCENARIO_METHOD(Fixture, "GameRepr empty serialization") {
    GIVEN("an empty game and empty players") {
        auto extra = std::make_shared<extra_data::GameExtraData>(); // no maps, no loot
        model::Game original_game(extra);
        app::Players original_players(original_game); // empty

        WHEN("the game and players are serialized via GameRepr") {
            serialize_game_save::GameRepr repr(original_game, original_players);
            output_archive << repr;

            THEN("they can be deserialized and fully restored") {
                strm.seekg(0);
                InputArchive input_archive{strm};
                serialize_game_save::GameRepr restored_repr;
                input_archive >> restored_repr;

                model::Game restored_game(extra);
                app::Players restored_players(restored_game);
                boost::asio::io_context restored_ioc;

                restored_repr.Restore(restored_game, restored_players, restored_ioc);

                CHECK(restored_game.GetSessions().empty());
                CHECK(restored_players.GetAllPlayers().empty());
            }
        }
    }
}