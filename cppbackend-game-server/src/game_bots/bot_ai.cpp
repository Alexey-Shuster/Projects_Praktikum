#include "bot_ai.h"
#include <algorithm>
#include <cmath>
#include "../common/utils.h"  // for CalculateDistance2D

namespace model {

std::optional<app_geom::Direction2D> BotAI::UpdateDirection(
    const Dog& dog,
    const Map& map,
    const RoadGraph& road_graph,
    const BotWorldState& world_state,
    std::mt19937& rng,
    std::chrono::milliseconds time_delta)
{
    // ------------------------------------------------------------------
    // 1. Update internal timer (used for roaming timeout)
    // ------------------------------------------------------------------
    time_in_state_ += time_delta;

    // ------------------------------------------------------------------
    // 2. Detect important events via bag size changes
    // ------------------------------------------------------------------
    // If bag becomes full → go to nearest office
    if (dog.GetBag().size() >= map.GetDefaultCapacity()) {
        state_ = State::MOVING_TO_OFFICE;
        ResetTargetAndTime();   // discard old target, reset timer
    }
    // If bag becomes empty → we must have reached an office (or started empty) → roam
    else if (dog.GetBag().empty()) {
        state_ = State::ROAMING;
        ResetTargetAndTime();
    }
    last_bag_size_ = dog.GetBag().size();   // store for future use (if needed)

    // ------------------------------------------------------------------
    // 3. Possibly switch from roaming to loot seeking (randomly)
    // ------------------------------------------------------------------
    UpdateStateFromWorld(dog, world_state, rng);

    // ------------------------------------------------------------------
    // 4. If we have no target, decide what to do based on current state
    // ------------------------------------------------------------------
    if (!target_) {
        switch (state_) {
            case State::ROAMING:
                // Roam for a while, then pick a new random point on the road network
                if (time_in_state_ >= MAX_ROAM_TIME) {
                    ChooseNewRoamingTarget(dog, map, rng);
                    time_in_state_ = std::chrono::milliseconds::zero();
                }
                break;

            case State::MOVING_TO_LOOT:
                if (!world_state.loot_positions.empty()) {
                    // Go to the closest loot pile
                    auto closest = std::ranges::min_element(world_state.loot_positions,
                        [&](const auto& a, const auto& b) {
                            return utils::CalculateDistance2D(dog.GetPosition(), a) <
                                   utils::CalculateDistance2D(dog.GetPosition(), b);
                        });
                    target_ = *closest;
                } else {
                    // No loot visible → fall back to roaming
                    RoamingToPoint(map.GetRandomPoint());
                }
                break;

            case State::MOVING_TO_OFFICE:
                if (!world_state.office_positions.empty()) {
                    // Go to the closest office
                    auto closest = std::ranges::min_element(world_state.office_positions,
                        [&](const auto& a, const auto& b) {
                            return utils::CalculateDistance2D(dog.GetPosition(), a) <
                                   utils::CalculateDistance2D(dog.GetPosition(), b);
                        });
                    target_ = *closest;
                } else {
                    // No offices (shouldn't happen) → roam
                    RoamingToPoint(map.GetRandomPoint());
                }
                break;
        }

        // If we now have a target, compute a path to it
        if (target_) {
            PlanPathToTarget(dog, road_graph);
        }
    }

    // ------------------------------------------------------------------
    // 5. If path planning failed, give up and go back to roaming
    // ------------------------------------------------------------------
    if (target_ && path_.empty()) {
        target_.reset();
        state_ = State::ROAMING;
        time_in_state_ = std::chrono::milliseconds::zero();
    }

    // ------------------------------------------------------------------
    // 6. Follow the path (if any) and determine the desired direction
    // ------------------------------------------------------------------
    std::optional<app_geom::Direction2D> desired_dir = std::nullopt;

    if (target_ && !path_.empty()) {
        const app_geom::Position2D current = dog.GetPosition();

        // Advance along the path until we find a waypoint we haven't reached yet
        while (path_index_ < path_.size()) {
            const app_geom::Position2D& waypoint = path_[path_index_];
            double dist = utils::CalculateDistance2D(current, waypoint);

            // If we are close enough to the current waypoint, move to the next one
            if (dist < kWaypointReachDist) {
                ++path_index_;
                // Skip any subsequent waypoints that are also very close
                while (path_index_ < path_.size() &&
                       utils::CalculateDistance2D(current, path_[path_index_]) < kWaypointReachDist) {
                    ++path_index_;
                }
            } else {
                // Not yet at this waypoint → stop advancing
                break;
            }
        }

        // After advancing, check if we have reached the final target
        if (path_index_ >= path_.size()) {
            // Path finished – target is considered reached
            target_.reset();
            path_.clear();
            path_index_ = 0;
            // No movement needed this tick
            desired_dir = std::nullopt;
        } else {
            // Still have a waypoint ahead – move towards it
            desired_dir = DirectionToWaypoint(current, path_[path_index_]);
        }
    }

    // ------------------------------------------------------------------
    // 7. If no new direction, try to reuse the last valid direction
    //    (helps with smooth movement when waiting for a new path)
    // ------------------------------------------------------------------
    if (!desired_dir) {
        if (last_direction_ && reuse_counter_ < kMaxDirectionReuse) {
            if (IsDirectionValid(dog, map, *last_direction_)) {
                ++reuse_counter_;
                return *last_direction_;
            }
        }
        return std::nullopt;
    }

    // ------------------------------------------------------------------
    // 8. We have a new desired direction – update state and return it
    // ------------------------------------------------------------------
    last_direction_ = desired_dir;
    reuse_counter_ = 0;
    return desired_dir;
}

void BotAI::ChooseNewRoamingTarget(const Dog& dog, const Map& map, std::mt19937& rng) {
    const auto& roads = map.GetRoads();
    if (roads.empty()) return;

    std::uniform_int_distribution<size_t> road_choice(0, roads.size() - 1);
    const auto& road = roads[road_choice(rng)];

    double start, end;
    if (road.IsHorizontal()) {
        start = road.GetStart().x;
        end = road.GetEnd().x;
    } else {
        start = road.GetStart().y;
        end = road.GetEnd().y;
    }
    if (start > end) std::swap(start, end);
    std::uniform_real_distribution<double> coord_dist(start, end);
    double coord = coord_dist(rng);

    app_geom::Position2D target;
    if (road.IsHorizontal()) {
        target = {coord, static_cast<double>(road.GetStart().y)};
    } else {
        target = {static_cast<double>(road.GetStart().x), coord};
    }
    target_ = target;
}

void BotAI::PlanPathToTarget(const Dog& dog, const RoadGraph& road_graph) {
    if (!target_) {
        path_.clear();
        return;
    }
    path_ = road_graph.FindPath(dog.GetPosition(), *target_);
    path_index_ = 0;
}

void BotAI::UpdateStateFromWorld(const Dog& dog, const BotWorldState& world_state, std::mt19937& rng) {
    // If bag not full and we are roaming, occasionally switch to loot seeking
    if (state_ == State::ROAMING &&
        dog.GetBag().size() < dog.GetMap()->GetDefaultCapacity() &&
        !world_state.loot_positions.empty())
    {
        static std::uniform_real_distribution<double> prob(0.0, 1.0);
        if (prob(rng) < common_values::DEFAULT_LOOT_CONFIG_PROBABILITY) {
            state_ = State::MOVING_TO_LOOT;
            ResetTargetAndTime();
        }
    }
}

app_geom::Direction2D BotAI::DirectionToWaypoint(const app_geom::Position2D& from,
                                                  const app_geom::Position2D& to) {
    double dx = to.x - from.x;
    double dy = to.y - from.y;
    if (std::abs(dx) > std::abs(dy)) {
        return dx > 0 ? app_geom::Direction2D::RIGHT : app_geom::Direction2D::LEFT;
    } else {
        return dy > 0 ? app_geom::Direction2D::DOWN : app_geom::Direction2D::UP;
    }
}

void BotAI::ResetTargetAndTime() {
    target_.reset();
    time_in_state_ = std::chrono::milliseconds::zero();
}

void BotAI::RoamingToPoint(const app_geom::Position2D &point) {
    state_ = State::ROAMING;
    target_ = point;
}

bool BotAI::IsDirectionValid(const Dog &dog, const Map &map, app_geom::Direction2D dir) {
    auto roads = map.GetRoadEngine().FindRoadsAtPosition(dog.GetPosition());
    if (roads.empty()) return false;
    if (roads.size() > 1) return true;  // intersection – any direction allowed

    const auto& road = *roads.front();
    if (road.IsHorizontal()) {
        return dir == app_geom::Direction2D::LEFT || dir == app_geom::Direction2D::RIGHT;
    } else {
        return dir == app_geom::Direction2D::UP || dir == app_geom::Direction2D::DOWN;
    }
}

} // namespace model