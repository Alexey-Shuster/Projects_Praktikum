#pragma once

#include <optional>
#include <chrono>
#include <vector>
#include <random>

#include "../common/game_utils/geometry.h"
#include "../game_model/game_map.h"
#include "../game_model/road_engine/road_graph.h"
#include "../game_model/dog.h"

namespace model {

    struct BotWorldState {
        std::vector<app_geom::Position2D> loot_positions;
        std::vector<app_geom::Position2D> office_positions;
    };

    class BotAI {
    public:
        // Called every tick to decide the next direction.
        // Returns std::nullopt if the bot should stop, otherwise a direction.
        std::optional<app_geom::Direction2D> UpdateDirection(
            const Dog& dog,
            const Map& map,
            const RoadGraph& road_graph,
            const BotWorldState& world_state,
            std::mt19937& rng,
            std::chrono::milliseconds time_delta);

    private:
        enum class State {
            ROAMING,
            MOVING_TO_LOOT,
            MOVING_TO_OFFICE
        };
        State state_ = State::ROAMING;

        std::optional<app_geom::Position2D> target_;
        std::vector<app_geom::Position2D> path_;
        size_t path_index_ = 0;
        size_t last_bag_size_ = 0;
        std::chrono::milliseconds time_in_state_{0};
        static constexpr std::chrono::milliseconds MAX_ROAM_TIME{5000};

        // smoother bots movement
        std::optional<app_geom::Direction2D> last_direction_;
        int reuse_counter_ = 0;
        static constexpr int kMaxDirectionReuse = 5;        // reuse last direction up to 5 times
        static constexpr double kWaypointReachDist = 0.2;   // distance to consider a waypoint reached
        static constexpr double kEpsilon = 0.1;              // tolerance for waypoint arrival

        void ChooseNewRoamingTarget(const Dog& dog, const Map& map, std::mt19937& rng);
        void PlanPathToTarget(const Dog& dog, const RoadGraph& road_graph);
        void UpdateStateFromWorld(const Dog& dog, const BotWorldState& world_state, std::mt19937& rng);

        void ResetTargetAndTime();
        void RoamingToPoint(const app_geom::Position2D& point);

        static app_geom::Direction2D DirectionToWaypoint(const app_geom::Position2D& from,
                                                         const app_geom::Position2D& to);
        static bool IsDirectionValid(const Dog& dog, const Map& map, app_geom::Direction2D dir);
    };

} // namespace model