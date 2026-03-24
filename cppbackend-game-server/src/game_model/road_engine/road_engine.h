#pragma once

#include "road_graph.h"

#include <map>

namespace model {

    struct Point2DPairHasher {
        std::size_t operator()(const std::pair<model_geom::Point2D, model_geom::Point2D>& point_pair) const noexcept {
            std::size_t h1 = std::hash<model_geom::Coord>{}(point_pair.first.x);
            std::size_t h2 = std::hash<model_geom::Coord>{}(point_pair.first.y);
            std::size_t h3 = std::hash<model_geom::Coord>{}(point_pair.second.x);
            std::size_t h4 = std::hash<model_geom::Coord>{}(point_pair.second.y);

            // Simple XOR approach (not ideal for all cases but works for many)
            return h1 ^ (h2 << 1) ^ (h3 << 2) ^ (h4 << 3);
        }
    };

// RoadIndex for efficient road lookup
class RoadEngine {
public:
    using RoadBounds = app_geom::Rectangle2DTwoPoints;
    using RoadIndex = std::pair<model_geom::Point2D, model_geom::Point2D>;

    // Make RoadIndex as std::pair{road->GetStart(), road->GetEnd()}
    static RoadIndex MakeRoadIndex(const Road* road);

    // Calculate RoadBounds for Road
    static RoadBounds CalculateRoadBounds(const Road* road);

    // Get RoadBounds for Road from data storage - calculate otherwise
    RoadBounds GetRoadBounds(const Road* road) const;

    bool IsPositionOnRoad(const app_geom::Position2D& pos,
                            const Road* road) const;

    // Check if position is at road bounds
    bool IsAtRoadBounds(const app_geom::Position2D& pos,
                            const Road* road) const;

    // Check if position is at road bounds in a specific direction
    bool IsAtRoadBounds(const app_geom::Position2D& pos,
                            const Road* road,
                            app_geom::Direction2D dir) const;

    // Check if road fits movement direction (for intersections)
    static bool IsMoveDirectionFitsRoad(app_geom::Direction2D dir,
                                            const Road* road);

    // Check if road fits movement direction
    // (for complex intersections with separate Road for each Direction)
    bool IsRoadFitsMove(app_geom::Direction2D dir,
                        const app_geom::Position2D& target,
                        const Road* road) const;

    app_geom::Position2D ClampPositionToRoadBounds(const app_geom::Position2D& target,
                                                    const Road* road) const;

    // Build RoadIndex for Map`s roads
    void BuildIndex(const std::vector<Road>& roads);

    // Find nearest road at given position (within road width tolerance)
    std::vector<const Road*> FindRoadsAtPosition(const app_geom::Position2D& pos) const;

    // Choose road based on current and target positions
    const Road* ChooseRoadForMovement(const app_geom::Position2D& from,
                                        const app_geom::Position2D& to,
                                        app_geom::Direction2D dir) const;

    // Choose road at intersection based on target position
    const Road* ChooseRoadAtIntersection(const std::vector<const Road*>& roads,
                                         const app_geom::Position2D& to,
                                         app_geom::Direction2D dir) const;

private:
    // Horizontal Roads Index where key: y-coordinate
    std::map<model_geom::Coord, std::vector<const Road*>> horizontal_roads_;
    // Vertical Roads Index where key: x-coordinate
    std::map<model_geom::Coord, std::vector<const Road*>> vertical_roads_;
    // RoadBounds for Roads where key: RoadIndex
    mutable std::unordered_map<RoadIndex, RoadBounds, Point2DPairHasher> road_bounds_;
};

} // namespace model
