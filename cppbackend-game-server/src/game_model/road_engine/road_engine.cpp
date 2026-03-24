#include <algorithm>
#include <cstdlib>

#include "road_engine.h"

#include "../../common/constants.h"
#include "../../common/utils.h"

namespace model {

RoadEngine::RoadBounds RoadEngine::CalculateRoadBounds(const Road *road)
{
    double min_x = std::min(road->GetStart().x, road->GetEnd().x) - common_values::ROAD_WIDTH_HALF;
    double max_x = std::max(road->GetStart().x, road->GetEnd().x) + common_values::ROAD_WIDTH_HALF;

    double min_y = std::min(road->GetStart().y, road->GetEnd().y) - common_values::ROAD_WIDTH_HALF;
    double max_y = std::max(road->GetStart().y, road->GetEnd().y) + common_values::ROAD_WIDTH_HALF;

    app_geom::Position2D top_left{min_x, min_y};
    app_geom::Position2D bottom_right{max_x, max_y};

    return {top_left, bottom_right};
}

RoadEngine::RoadBounds RoadEngine::GetRoadBounds(const Road *road) const
{
    if (const auto road_bounds_it = road_bounds_.find(MakeRoadIndex(road)); road_bounds_it != road_bounds_.end()) {
        return road_bounds_it->second;
    }
    return CalculateRoadBounds(road);
}

RoadEngine::RoadIndex RoadEngine::MakeRoadIndex(const Road *road)
{
    return {road->GetStart(), road->GetEnd()};
}

bool RoadEngine::IsPositionOnRoad(const app_geom::Position2D &pos, const Road *road) const {
    auto road_bounds = GetRoadBounds(road);

    if (road->IsHorizontal())
    {
        double road_y = road->GetStart().y;

        // For horizontal road, position must be at road y-level ± tolerance
        return utils::IsDiffInsideBounds(pos.y, road_y, common_values::ROAD_WIDTH_HALF) &&
                    pos.x >= road_bounds.top_left.x - common_values::DOUBLE_ABS_TOLERANCE &&
                    pos.x <= road_bounds.bottom_right.x + common_values::DOUBLE_ABS_TOLERANCE;
    }
    else if (road->IsVertical())
    {
        double road_x = road->GetStart().x;

        // For vertical road, position must be at road x-level ± tolerance
        return utils::IsDiffInsideBounds(pos.x, road_x, common_values::ROAD_WIDTH_HALF) &&
                    pos.y >= road_bounds.top_left.y - common_values::DOUBLE_ABS_TOLERANCE &&
                    pos.y <= road_bounds.bottom_right.y + common_values::DOUBLE_ABS_TOLERANCE;
    }
    return false;
}

bool RoadEngine::IsAtRoadBounds(const app_geom::Position2D &pos, const Road *road) const
{
    auto road_bounds = GetRoadBounds(road);

    return  utils::IsWithinTolerance(pos.x, road_bounds.top_left.x)
           || utils::IsWithinTolerance(pos.x, road_bounds.bottom_right.x)
           || utils::IsWithinTolerance(pos.y, road_bounds.top_left.y)
           || utils::IsWithinTolerance(pos.y, road_bounds.bottom_right.y);
}

bool RoadEngine::IsAtRoadBounds(const app_geom::Position2D &pos, const Road *road, app_geom::Direction2D dir) const
{
    auto road_bounds = GetRoadBounds(road);

    switch(dir) {
    case app_geom::Direction2D::LEFT:
        // Check if at left boundary
        return utils::IsWithinTolerance(pos.x, road_bounds.top_left.x);

    case app_geom::Direction2D::RIGHT:
        // Check if at right boundary
        return utils::IsWithinTolerance(pos.x, road_bounds.bottom_right.x);

    case app_geom::Direction2D::UP:
        // Check if at top boundary
        return utils::IsWithinTolerance(pos.y, road_bounds.top_left.y);

    case app_geom::Direction2D::DOWN:
        // Check if at bottom boundary
        return utils::IsWithinTolerance(pos.y, road_bounds.bottom_right.y);

    // case app_geom::Direction::STOP no check: if Stop - no Move - Speed is Zero
    default:
        return false;
    }
}

bool RoadEngine::IsMoveDirectionFitsRoad(app_geom::Direction2D dir, const Road *road) {
    // Try to find road that fits movement direction
    if (road->IsHorizontal() &&
        (dir == app_geom::Direction2D::LEFT || dir == app_geom::Direction2D::RIGHT))
    {
        return true;    // Horizontal movement fits to horizontal road
    } else
        if (road->IsVertical() &&
             (dir == app_geom::Direction2D::UP || dir == app_geom::Direction2D::DOWN))
    {
        return true;    // Vertical movement fits to vertical road
    }

    return false;
}

bool RoadEngine::IsRoadFitsMove(app_geom::Direction2D dir, const app_geom::Position2D &target, const Road *road) const
{
    // First check if road orientation matches movement direction
    if (!IsMoveDirectionFitsRoad(dir, road)) {
        return false;
    }

    // Start Position checked earlier in FindRoadsAtPosition
    // Thus all roads here are valid according to Move start Position

    // Then check if target position is on the road
    return IsPositionOnRoad(target, road);
}

app_geom::Position2D RoadEngine::ClampPositionToRoadBounds(const app_geom::Position2D &target, const Road *road) const {
    auto road_bounds = GetRoadBounds(road);

    double clamped_x = std::clamp(target.x, road_bounds.top_left.x, road_bounds.bottom_right.x);
    double clamped_y = std::clamp(target.y, road_bounds.top_left.y, road_bounds.bottom_right.y);

    return {clamped_x, clamped_y};
}

void RoadEngine::BuildIndex(const std::vector<Road> &roads) {
    // Clear existing index
    horizontal_roads_.clear();
    vertical_roads_.clear();

    for (const auto& road : roads) {
        if (road.IsHorizontal())
        {
            auto y = road.GetStart().y;
            // Index horizontal roads by y-coordinate
            horizontal_roads_[y].push_back(&road);
        }
        else if (road.IsVertical())
        {
            // Index vertical roads by x-coordinate
            auto x = road.GetStart().x;
            vertical_roads_[x].push_back(&road);
        }
        // Index RoadBounds for roads by RoadIndex
        road_bounds_[MakeRoadIndex(&road)] = CalculateRoadBounds(&road);
    }
}

std::vector<const Road *> RoadEngine::FindRoadsAtPosition(const app_geom::Position2D &pos) const {
    std::vector<const Road*> result;

    // Check horizontal roads
    for (const auto& [y, roads] : horizontal_roads_) {
        if (utils::IsDiffInsideBounds(y, pos.y, common_values::ROAD_WIDTH_HALF)) {
            for (const auto* road : roads) {
                if (IsPositionOnRoad(pos, road)) {
                    result.push_back(road);
                }
            }
        }
    }

    // Check vertical roads
    for (const auto& [x, roads] : vertical_roads_) {
        if (utils::IsDiffInsideBounds(x, pos.x, common_values::ROAD_WIDTH_HALF)) {
            for (const auto* road : roads) {
                if (IsPositionOnRoad(pos, road)) {
                    result.push_back(road);
                }
            }
        }
    }

    return result;
}

const Road *RoadEngine::ChooseRoadForMovement(const app_geom::Position2D &from, const app_geom::Position2D &to,
                                                app_geom::Direction2D dir) const {
    // Get all roads at current position
    auto roads_at_pos = FindRoadsAtPosition(from);

    // No Road for Position
    if (roads_at_pos.empty()) {
        std::string er_text = "No Road found for start Position";
        boost_logger::LogError(EXIT_FAILURE, er_text, "RoadEngine::ChooseRoadForMovement");
        throw std::runtime_error(er_text);
    }

    // Try to find Road that fits target position
    for (const auto& road : roads_at_pos)
    {
        // Start position is already on Road - check done earlier at RoadIndex::FindRoadsAtPosition
        if (IsRoadFitsMove(dir, to, road)) {
            return road;
        }
    }
    return nullptr;
}

const Road *RoadEngine::ChooseRoadAtIntersection(const std::vector<const Road *> &roads, const app_geom::Position2D &to, app_geom::Direction2D dir) const
{
    return nullptr;
}

} // namespace model
