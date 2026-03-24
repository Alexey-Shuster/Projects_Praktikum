#include "road_graph.h"
#include "road_engine.h"
#include "../../common/utils.h"

#include <unordered_set>
#include <stdexcept>
#include <cassert>
#include <ranges>

namespace model {

RoadGraph::RoadGraph(const std::vector<Road>& roads, const RoadEngine& engine)
    : roads_(roads)                    // store pointer to original vector
    , road_engine_(engine)
{
    // Build mapping from original Road* to index
    for (size_t i = 0; i < roads_.size(); ++i) {
        road_to_index_[&(roads_)[i]] = i;   // key = address in original vector
    }

    // --- Step 1: Collect all unique node positions ---
    std::unordered_set<app_geom::Position2D, app_geom::Position2DHasher> unique_points;

    // Add road endpoints (using original roads)
    for (const auto& road : roads_) {
        unique_points.insert(utils::Point2DToPosition2D(road.GetStart()));
        unique_points.insert(utils::Point2DToPosition2D(road.GetEnd()));
    }

    // Add intersections (where a horizontal and vertical road cross)
    std::vector<const Road*> horizontal;
    std::vector<const Road*> vertical;
    for (const auto& road : roads_) {
        if (road.IsHorizontal())
            horizontal.push_back(&road);
        else if (road.IsVertical())
            vertical.push_back(&road);
    }

    for (const auto* h_road : horizontal) {
        double y = h_road->GetStart().y;

        for (const auto* v_road : vertical) {
            double x = v_road->GetStart().x;

            app_geom::Position2D candidate{x, y};
            if (road_engine_.IsPositionOnRoad(candidate, h_road) &&
                road_engine_.IsPositionOnRoad(candidate, v_road)) {
                unique_points.insert(candidate);
            }
        }
    }

    // --- Step 2: Create nodes for all unique points ---
    for (const auto& pt : unique_points) {
        AddNode(pt);
    }

    // --- Step 3: For each road, collect nodes that lie on it and sort them ---
    for (size_t ri = 0; ri < roads_.size(); ++ri) {
        const auto& road = (roads_)[ri];
        std::vector<size_t> nodes_on_road;
        for (size_t i = 0; i < nodes_.size(); ++i) {
            if (road_engine_.IsPositionOnRoad(nodes_[i].pos, &road)) {
                nodes_on_road.push_back(i);
            }
        }

        // Sort along the road
        if (road.IsHorizontal()) {
            std::ranges::sort(nodes_on_road,
                              [this](size_t a, size_t b) {
                                  return nodes_[a].pos.x < nodes_[b].pos.x;
                              });
        } else {
            std::ranges::sort(nodes_on_road,
                              [this](size_t a, size_t b) {
                                  return nodes_[a].pos.y < nodes_[b].pos.y;
                              });
        }

        road_nodes_[ri] = std::move(nodes_on_road);
    }

    // --- Step 3b: Validate endpoints ---
    for (size_t ri = 0; ri < roads_.size(); ++ri) {
        if (road_nodes_[ri].size() < 2) {
            throw std::runtime_error("RoadGraph: road " + std::to_string(ri) +
                                     " has fewer than 2 nodes (endpoints missing)");
        }
    }

    // --- Step 4: Build edges between consecutive nodes on each road ---
    for (size_t ri = 0; ri < roads_.size(); ++ri) {
        const auto& nodes_on_road = road_nodes_[ri];
        for (size_t k = 0; k + 1 < nodes_on_road.size(); ++k) {
            size_t u = nodes_on_road[k];
            size_t v = nodes_on_road[k + 1];
            double dist = std::hypot(
                nodes_[v].pos.x - nodes_[u].pos.x,
                nodes_[v].pos.y - nodes_[u].pos.y);
            adj_[u].emplace_back(v, dist);
            adj_[v].emplace_back(u, dist);
        }
    }
}

size_t RoadGraph::AddNode(const app_geom::Position2D& pos) {
    // Check if node already exists
    auto it = pos_to_index_.find(pos);
    if (it != pos_to_index_.end())
        return it->second;

    // Create new node
    size_t idx = nodes_.size();
    nodes_.push_back({pos});
    pos_to_index_[pos] = idx;
    adj_.emplace_back();    // empty adjacency list for this node
    return idx;
}

std::vector<size_t> RoadGraph::FindRoadIndicesAtPoint(const app_geom::Position2D& point) const {
    std::vector<const Road*> roads_at_point = road_engine_.FindRoadsAtPosition(point);
    std::vector<size_t> indices;
    indices.reserve(roads_at_point.size());

    for (const auto* road : roads_at_point) {
        auto it = road_to_index_.find(road);
        if (it != road_to_index_.end()) {
            indices.push_back(it->second);
        }
    }
    return indices;
}

std::pair<size_t, size_t> RoadGraph::FindNeighborNodesOnRoad(
    const app_geom::Position2D& point,
    size_t road_idx) const
{
    auto it = road_nodes_.find(road_idx);
    if (it == road_nodes_.end() || it->second.empty())
        return {SIZE_MAX, SIZE_MAX};

    const auto& nodes_on_road = it->second;
    const Road& road = (roads_)[road_idx];

    if (road.IsHorizontal()) {
        // Binary search by x coordinate
        auto it_idx = std::ranges::lower_bound(nodes_on_road, point.x,
            [this](size_t idx, double x) {
                return nodes_[idx].pos.x < x;
            });

        if (it_idx == nodes_on_road.begin()) {
            // point is before the first node
            return {SIZE_MAX, *it_idx};
        } else if (it_idx == nodes_on_road.end()) {
            // point is after the last node
            return {nodes_on_road.back(), SIZE_MAX};
        } else {
            size_t right = *it_idx;
            size_t left = *(it_idx - 1);
            // Check if point coincides with left or right node
            if (std::abs(nodes_[left].pos.x - point.x) <= common_values::DOUBLE_ABS_TOLERANCE)
                return {left, left};
            if (std::abs(nodes_[right].pos.x - point.x) <= common_values::DOUBLE_ABS_TOLERANCE)
                return {right, right};
            return {left, right};
        }
    } else { // vertical road
        // Binary search by y coordinate
        auto it_idx = std::ranges::lower_bound(nodes_on_road, point.y,
            [this](size_t idx, double y) {
                return nodes_[idx].pos.y < y;
            });

        if (it_idx == nodes_on_road.begin()) {
            return {SIZE_MAX, *it_idx};
        } else if (it_idx == nodes_on_road.end()) {
            return {nodes_on_road.back(), SIZE_MAX};
        } else {
            size_t right = *it_idx;
            size_t left = *(it_idx - 1);
            if (std::abs(nodes_[left].pos.y - point.y) <= common_values::DOUBLE_ABS_TOLERANCE)
                return {left, left};
            if (std::abs(nodes_[right].pos.y - point.y) <= common_values::DOUBLE_ABS_TOLERANCE)
                return {right, right};
            return {left, right};
        }
    }
}

double RoadGraph::DistanceAlongRoad(const app_geom::Position2D& point,
                                    const app_geom::Position2D& node,
                                    size_t road_idx) const
{
    const Road& road = roads_[road_idx];
    if (road.IsHorizontal()) {
        return std::abs(point.x - node.x);
    } else {
        return std::abs(point.y - node.y);
    }
}

std::vector<app_geom::Position2D> RoadGraph::FindPath(
    const app_geom::Position2D& start_pos,
    const app_geom::Position2D& goal_pos) const
{
    // --- Validate that start and goal are on the road network ---
    auto start_roads = FindRoadIndicesAtPoint(start_pos);
    auto goal_roads = FindRoadIndicesAtPoint(goal_pos);
    if (start_roads.empty() || goal_roads.empty())
        return {};

    // --- Create temporary node indices for start and goal ---
    const size_t start_temp = nodes_.size();      // index of temporary start node
    const size_t goal_temp = nodes_.size() + 1;   // index of temporary goal node

    // --- Build edges from start_temp to nodes on the roads at start_pos ---
    std::vector<std::pair<size_t, double>> start_edges;
    for (size_t road_idx : start_roads) {
        auto [left, right] = FindNeighborNodesOnRoad(start_pos, road_idx);
        if (left != SIZE_MAX) {
            double dist = DistanceAlongRoad(start_pos, nodes_[left].pos, road_idx);
            start_edges.emplace_back(left, dist);
        }
        if (right != SIZE_MAX && right != left) {
            double dist = DistanceAlongRoad(start_pos, nodes_[right].pos, road_idx);
            start_edges.emplace_back(right, dist);
        }
    }

    // --- Build edges from nodes on the roads at goal_pos to goal_temp ---
    std::unordered_map<size_t, double> goal_conn;
    for (size_t road_idx : goal_roads) {
        auto [left, right] = FindNeighborNodesOnRoad(goal_pos, road_idx);
        if (left != SIZE_MAX) {
            double dist = DistanceAlongRoad(goal_pos, nodes_[left].pos, road_idx);
            auto it = goal_conn.find(left);
            if (it == goal_conn.end() || dist < it->second)
                goal_conn[left] = dist;
        }
        if (right != SIZE_MAX && right != left) {
            double dist = DistanceAlongRoad(goal_pos, nodes_[right].pos, road_idx);
            auto it = goal_conn.find(right);
            if (it == goal_conn.end() || dist < it->second)
                goal_conn[right] = dist;
        }
    }

    if (start_edges.empty() || goal_conn.empty())
        return {};

    // --- A* search from start_temp to goal_temp ---
    const size_t total_nodes = nodes_.size() + 2; // permanent + two temps
    std::vector<double> g(total_nodes, std::numeric_limits<double>::max());
    std::vector<size_t> came_from(total_nodes, SIZE_MAX);
    std::vector<double> f(total_nodes, std::numeric_limits<double>::max());

    auto heuristic = [&](size_t idx) -> double {
        if (idx == goal_temp) return 0.0;
        if (idx == start_temp) {
            return std::hypot(start_pos.x - goal_pos.x, start_pos.y - goal_pos.y);
        } else {
            // Defensive: idx must be a permanent node
            assert(idx < nodes_.size());
            return std::hypot(nodes_[idx].pos.x - goal_pos.x,
                              nodes_[idx].pos.y - goal_pos.y);
        }
    };

    using QueueElement = std::pair<double, size_t>;
    std::priority_queue<QueueElement, std::vector<QueueElement>, std::greater<>> pq;

    g[start_temp] = 0.0;
    f[start_temp] = heuristic(start_temp);
    pq.emplace(f[start_temp], start_temp);

    while (!pq.empty()) {
        auto [current_f, u] = pq.top();
        pq.pop();
        if (u == goal_temp)
            break;
        if (current_f > f[u] + 1e-9)
            continue;

        // Explore neighbors
        if (u < nodes_.size()) { // permanent node
            // Edges to other permanent nodes
            for (auto [v, w] : adj_[u]) {
                assert(v < nodes_.size()); // v must be permanent
                double tentative_g = g[u] + w;
                if (tentative_g < g[v]) {
                    g[v] = tentative_g;
                    came_from[v] = u;
                    f[v] = g[v] + heuristic(v);
                    pq.emplace(f[v], v);
                }
            }
            // Edge to goal_temp if this node is connected to goal
            auto goal_it = goal_conn.find(u);
            if (goal_it != goal_conn.end()) {
                double w = goal_it->second;
                double tentative_g = g[u] + w;
                if (tentative_g < g[goal_temp]) {
                    g[goal_temp] = tentative_g;
                    came_from[goal_temp] = u;
                    f[goal_temp] = g[goal_temp] + heuristic(goal_temp);
                    pq.emplace(f[goal_temp], goal_temp);
                }
            }
        } else if (u == start_temp) {
            for (auto [v, w] : start_edges) {
                assert(v < nodes_.size()); // v must be permanent
                double tentative_g = g[u] + w;
                if (tentative_g < g[v]) {
                    g[v] = tentative_g;
                    came_from[v] = u;
                    f[v] = g[v] + heuristic(v);
                    pq.emplace(f[v], v);
                }
            }
        }
        // goal_temp has no outgoing edges
    }

    if (came_from[goal_temp] == SIZE_MAX)
        return {};

    // --- Reconstruct path from goal_temp back to start_temp ---
    std::vector<size_t> path_indices;
    for (size_t v = goal_temp; v != SIZE_MAX; v = came_from[v]) {
        path_indices.push_back(v);
    }
    std::ranges::reverse(path_indices);

    std::vector<app_geom::Position2D> path;
    for (size_t idx : path_indices) {
        if (idx == start_temp)
            path.push_back(start_pos);
        else if (idx == goal_temp)
            path.push_back(goal_pos);
        else {
            assert(idx < nodes_.size());
            path.push_back(nodes_[idx].pos);
        }
    }
    return path;
}

} // namespace model