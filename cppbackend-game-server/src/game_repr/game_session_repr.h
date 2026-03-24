#pragma once

#include "loot_repr.h"
#include "../game_model/game_model.h"

namespace serialize_game_save {

    // Representation of Dog
    class DogRepr {
    public:
        DogRepr() = default;

        explicit DogRepr(const model::Dog& dog)
            : id_(dog.GetId())
            , name_(dog.GetName())
            , pos_x_(dog.GetPosition().x)
            , pos_y_(dog.GetPosition().y)
            , speed_vx_(dog.GetSpeed().x)
            , speed_vy_(dog.GetSpeed().y)
            , dir_int_(static_cast<int>(dog.GetDirection()))
            , score_(dog.GetScore())
        {
            // Convert bag of pointers to vector of loot object IDs
            for (const auto* loot_obj : dog.GetBag()) {
                bag_loot_ids_.push_back(loot_obj->object_id);
            }
        }

        // Restore a Dog object, given the map and a per‑session loot ID → object pointer map
        model::Dog Restore(const model::Map* map,
                           const std::unordered_map<loot::LootObjectId, loot::LootObject*>& id_to_loot) const {
            // Reconstruct geometry types from primitives
            app_geom::Position2D pos{pos_x_, pos_y_};
            app_geom::Speed2D speed{speed_vx_, speed_vy_};
            app_geom::Direction2D dir = static_cast<app_geom::Direction2D>(dir_int_);

            model::Dog dog(id_, name_, map);
            dog.SetPosition(pos);
            dog.SetDirection(dir);
            dog.SetSpeed(speed);
            dog.AddScore(score_);

            // Restore bag using the per‑session loot map
            for (auto loot_id : bag_loot_ids_) {
                auto it = id_to_loot.find(loot_id);
                if (it != id_to_loot.end()) {
                    dog.AddToBag(it->second);
                }
            }

            // Restore current road using stored index
            const auto& roads = map->GetRoads();
            if (road_index_ < roads.size()) {
                dog.SetCurrentRoad(&roads[road_index_]);
            } else {
                dog.SetCurrentRoad(&map->GetZeroRoad());
            }

            return dog;
        }

    private:
        model::Dog::Id id_{};
        std::string name_;
        double pos_x_ = 0.0;
        double pos_y_ = 0.0;
        double speed_vx_ = 0.0;
        double speed_vy_ = 0.0;
        int dir_int_ = 0;           // 0=STOP, 1=LEFT, 2=RIGHT, 3=UP, 4=DOWN
        loot::Score score_{};
        std::vector<loot::LootObjectId> bag_loot_ids_;
        size_t road_index_ = 0;     // index into map's roads vector, 0 to be simple

        friend class boost::serialization::access;
        template <typename Archive>
        void serialize(Archive& ar, unsigned const /*version*/) {
            ar & id_;
            ar & name_;
            ar & pos_x_;
            ar & pos_y_;
            ar & speed_vx_;
            ar & speed_vy_;
            ar & dir_int_;
            ar & score_;
            ar & bag_loot_ids_;
            ar & road_index_;
        }
    };

    class GameSessionRepr {
    public:
        GameSessionRepr() = default;

        explicit GameSessionRepr(const model::GameSession& session)
            : session_id_(*session.GetId())
            , name_(session.GetName())
            , map_id_(*session.GetMap()->GetId())
        {
            for (const auto &dog: session.GetDogs() | std::views::values) {
                dogs_reprs_.emplace_back(dog);
            }
            // Pass map's loot types to LootStorageRepr constructor to compute indices
            loot_storage_repr_ = LootStorageRepr(session.GetLootStorage(),
                                                  session.GetMap()->GetId(),
                                                  session.GetGameExtraData());
        }

        void Restore(model::Game& game,
                     boost::asio::io_context& ioc,
                     std::shared_ptr<loot_gen::LootGenerator> loot_generator,
                     std::shared_ptr<extra_data::GameExtraData> extra_data,
                     std::unordered_map<model::GameSession::Id, model::GameSession*, model::Game::GameSessionIdHasher>& sessions_by_id) const
        {
            auto* map = game.FindMap(model::Map::Id(map_id_));
            if (!map) {
                std::string error_msg = "Failed to retrieve map from game";
                boost_logger::LogError(EXIT_FAILURE, error_msg, "GameSessionRepr::Restore");
                throw std::runtime_error(error_msg);
            };

            model::GameSession::Id id(session_id_);
            model::GameSession session(id, name_, map, ioc, loot_generator, extra_data, game.IsRetirementEnabled());

            // Restore loot storage first, obtaining a per‑session loot ID → object pointer map
            std::unordered_map<loot::LootObjectId, loot::LootObject*> id_to_loot;
            loot_storage_repr_.Restore(session.GetLootStorage(),
                                       extra_data->GetLootTypes(map->GetId()),
                                       id_to_loot);

            // Restore dogs using the per‑session loot map
            for (const auto& dog_repr : dogs_reprs_) {
                auto dog = dog_repr.Restore(map, id_to_loot);
                session.AddRestoredDog(std::move(dog));
            }

            // Insert session into game – this updates the session ID counter internally
            game.AddRestoredSession(std::move(session));

            // Retrieve pointer to the newly added session and store in map
            auto* session_ptr = game.FindGameSession(id);
            if (!session_ptr) {
                std::string error_msg = "Failed to retrieve restored session";
                boost_logger::LogError(EXIT_FAILURE, error_msg, "GameSessionRepr::Restore");
                throw std::runtime_error(error_msg);
            };
            sessions_by_id[id] = session_ptr;
        }

    private:
        uint32_t session_id_{};
        std::string name_;
        std::string map_id_;
        std::vector<DogRepr> dogs_reprs_;
        LootStorageRepr loot_storage_repr_;

        friend class boost::serialization::access;
        template <typename Archive>
        void serialize(Archive& ar, unsigned const /*version*/) {
            ar & session_id_;
            ar & name_;
            ar & map_id_;
            ar & dogs_reprs_;
            ar & loot_storage_repr_;
        }
    };

}// namespace serialize_game_save