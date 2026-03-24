#pragma once

#include "../../common/game_utils/geometry.h"
#include "../road.h"
#include <vector>
#include <unordered_map>
#include <queue>

namespace model {

// Forward declaration – we only need a reference in the header
class RoadEngine;

/**
 * RoadGraph builds a graph from the road network and provides shortest path queries.
 * It is designed specifically for bot navigation and does not depend on RoadEngine.
 *
 * Nodes are placed at:
 *   - All road endpoints
 *   - All intersections where a horizontal and vertical road cross.
 *
 * Edges are created between consecutive nodes along each road.
 *
 * Geometric checks (point on road, roads at a point) are delegated to an external
 * RoadEngine (typically owned by the Map) to avoid code duplication.
 */
class RoadGraph {
public:
    /**
     * Construct the graph from the given list of roads.
     * The roads are expected to be valid (start <= end coordinates).
     *
     * @param roads   List of roads that define the network.
     * @param engine  Reference to a RoadEngine already built with the same roads.
     *                It is used for all point‑on‑road queries and must outlive
     *                this RoadGraph.
     */
    explicit RoadGraph(const std::vector<Road>& roads, const RoadEngine& engine);

    /**
     * Find the shortest path along roads from start_pos to goal_pos.
     * Both positions must lie on at least one road (checked internally).
     * Returns a list of positions from start to goal inclusive, or empty if no path.
     */
    std::vector<app_geom::Position2D> FindPath(
        const app_geom::Position2D& start_pos,
        const app_geom::Position2D& goal_pos) const;

private:
    struct Node {
        app_geom::Position2D pos;
    };

    // Graph representation
    std::vector<Node> nodes_;                          // all permanent nodes
    std::unordered_map<app_geom::Position2D, size_t, app_geom::Position2DHasher> pos_to_index_;
    std::vector<std::vector<std::pair<size_t, double>>> adj_; // adjacency list (neighbor, distance)

    // For each road, store the indices of nodes that lie on it, sorted along the road.
    std::unordered_map<size_t, std::vector<size_t>> road_nodes_;  // key = road index

    // Original roads (for index mapping and graph building)
    const std::vector<Road>& roads_;

    // Reference to the external RoadEngine – must outlive this object.
    const RoadEngine& road_engine_;

    // Fast lookup from Road* to its index in roads_
    std::unordered_map<const Road*, size_t> road_to_index_;

    // --- Helper methods (using the external RoadEngine) ---

    /**
     * Add a node at the given position if not already present.
     * Returns the index of the node (existing or new).
     */
    size_t AddNode(const app_geom::Position2D& pos);

    /**
     * Find all road indices that contain the given point.
     * Delegates to RoadEngine::FindRoadsAtPosition and maps the results.
     */
    std::vector<size_t> FindRoadIndicesAtPoint(const app_geom::Position2D& point) const;

    /**
     * For a point on a specific road, find the two neighboring nodes along that road.
     * Returns (left_node_index, right_node_index) where:
     *   - left is the node with coordinate ≤ point (or SIZE_MAX if none)
     *   - right is the node with coordinate ≥ point (or SIZE_MAX if none)
     * If point is exactly at a node, both may be the same index.
     */
    std::pair<size_t, size_t> FindNeighborNodesOnRoad(
        const app_geom::Position2D& point,
        size_t road_idx) const;

    /**
     * Compute the distance along a road between a point and a node.
     * Assumes both are on the same road and the road is axis-aligned.
     */
    double DistanceAlongRoad(const app_geom::Position2D& point,
                             const app_geom::Position2D& node,
                             size_t road_idx) const;
};

} // namespace model