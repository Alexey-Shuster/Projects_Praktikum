#pragma once

#include <map>
#include <random>
#include <chrono>

#include "../game_model/dog.h"
#include "../game_model/game_map.h"
#include "../common/game_utils/geometry.h"
#include "../common/constants.h"
#include "bot_ai.h"
#include "../game_model/road_engine/road_graph.h"

namespace model {

    class BotManager {
    public:
        // Construct with the map bots belong to
        explicit BotManager(const Map* map);

        // Create the initial set of bots (half of MAX_PLAYERS_ON_MAP)
        void CreateBots();

        // Move all bots by the given time delta
        void MoveBots(std::chrono::milliseconds time_delta);

        // Update bot directions based on random decisions - simple bot-AI
        void UpdateDirections();

        // Update directions with world state and time delta - better bot-AI
        void UpdateDirections(const BotWorldState& world_state,
                              std::chrono::milliseconds time_delta);

        [[nodiscard]] const std::map<uint32_t, Dog>& GetBots() const noexcept { return bots_; }

        [[nodiscard]] size_t GetBotCount() const noexcept { return bots_.size(); }

        // Provide a vector of pointers to all bots (useful for collision processing).
        std::vector<Dog*> GetAllBotPointers();



    private:
        const Map* map_;                    // map on which bots move
        std::map<uint32_t, Dog> bots_;      // All bot dogs
        uint32_t next_bot_id_ = common_values::DOG_BOT_START_ID;        // Start above real player IDs
        std::mt19937 rng_;                  // Random generator for AI decisions
        std::map<uint32_t, BotAI> bot_ais_;
        RoadGraph road_graph_;

        // Predefined directions for convenience
        static inline std::vector<app_geom::Direction2D> all_dirs_ = {
            app_geom::Direction2D::UP,
            app_geom::Direction2D::DOWN,
            app_geom::Direction2D::LEFT,
            app_geom::Direction2D::RIGHT
        };
        static inline std::uniform_int_distribution<int> dir_dist_{0, 3};
        static inline std::uniform_real_distribution<double> change_prob_dist_{0.0, common_values::DEFAULT_DOG_SPEED};
    };

} // namespace model