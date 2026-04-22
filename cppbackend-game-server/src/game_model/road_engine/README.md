# Road Navigation Module for Game Server

The `/src/game_model/road_engine/` directory provides road-based movement constraints and pathfinding over the road network. `RoadEngine` handles road lookup, position validation, and movement clamping; `RoadGraph` builds a graph from roads and performs shortest‑path queries (used for AI bot navigation).

## Code Description

- **RoadEngine** (`road_engine.cpp/h`) – Efficient spatial index for roads. Builds separate indexes for horizontal and vertical roads by coordinate. Provides:
  - `IsPositionOnRoad()` – checks if a point lies on a given road (within half‑width tolerance).
  - `FindRoadsAtPosition()` – returns all roads containing a point.
  - `ChooseRoadForMovement()` – given a start position, target, and direction, selects the road that allows movement (respecting road orientation).
  - `ClampPositionToRoadBounds()` – snaps an out‑of‑bounds position back to the road’s bounding box.
  - Road bounds caching (bounding rectangles with half‑width) for performance.
- **RoadGraph** (`road_graph.cpp/h`) – Builds a graph from the road network for pathfinding. Nodes are placed at:
  - All road endpoints.
  - All intersections where a horizontal and vertical road cross.
  - Edges connect consecutive nodes along each road.
  - Implements A* search (`FindPath()`) to compute shortest paths between arbitrary points on the road network.
  - Uses `RoadEngine` for geometric queries (point‑on‑road, roads at a point) to avoid duplication.

## Patterns Used

- **Spatial Index** – `RoadEngine` uses `std::map` keyed by y‑coordinate (horizontal roads) and x‑coordinate (vertical roads) for O(log n) lookup of candidate roads.
- **Caching** – Road bounds are precomputed and stored in an `unordered_map` keyed by `(start, end)` pair.
- **Graph Construction** – `RoadGraph` builds nodes and edges from the road network, then applies A* for pathfinding.
- **Strategy / Delegation** – `RoadGraph` delegates point‑on‑road queries to an external `RoadEngine`, allowing reuse of the same geometric logic.
- **Temporary Nodes in Search** – During pathfinding, start and goal points are added as temporary nodes with edges to adjacent graph nodes, then removed after search.

## Libraries Used

- C++17 / C++20 STL – `vector`, `map`, `unordered_map`, `unordered_set`, `queue`, `algorithm`, `ranges`, `cmath`.
- Project‑internal – `common/utils.h` (tolerance checks, random, geometry conversions), `common/constants.h` (road half‑width, double tolerance), `common/boost_logger.h` (error logging).

## Files Summary

| File | Purpose |
|------|---------|
| `road_engine.h` | Declares `RoadEngine` class: spatial indexing, road bounds, position validation, movement road selection, clamping. |
| `road_engine.cpp` | Implements index building, road bounds calculation, geometric checks (point on road, at bounds), and road selection logic. |
| `road_graph.h` | Declares `RoadGraph` class: graph node/edge representation, A* pathfinding, helper methods for neighbor lookup. |
| `road_graph.cpp` | Constructs graph from roads (endpoints + intersections), builds adjacency lists, implements `FindPath()` using A* with temporary start/goal nodes. |

## Extra Data

### Integration with Dog Movement

`Dog::Move()` uses `RoadEngine` to ensure the dog never leaves the road network:

```cpp
// Inside Dog::Move()
road_to_move_ = map_->GetRoadEngine().ChooseRoadForMovement(pos_, target, dir_);
if (!road_to_move_) {
    pos_ = map_->GetRoadEngine().ClampPositionToRoadBounds(target, current_road);
    speed_ = Speed2D::Zero();
} else {
    pos_ = target;
}
```

### Bot Pathfinding with RoadGraph

Bots (AI players) use `RoadGraph` to plan routes to loot or offices:

```cpp
const auto& road_engine = map->GetRoadEngine();
RoadGraph graph(map->GetRoads(), road_engine);
auto path = graph.FindPath(current_pos, target_pos);
if (!path.empty()) {
    // follow path
}
```

### Road Bounds and Tolerance

Roads have a logical half‑width (`ROAD_WIDTH_HALF` from `constants.h`). A point is considered “on” a road if its distance to the road’s center line is ≤ this half‑width. Bounding boxes are expanded by the same amount. Tolerance for floating‑point comparisons uses `DOUBLE_ABS_TOLERANCE`.

### Graph Node Placement Example

Given a horizontal road from (0,0) to (10,0) and a vertical road from (5,-5) to (5,5), `RoadGraph` creates nodes at:
- Endpoints: (0,0), (10,0), (5,-5), (5,5)
- Intersection: (5,0)
Edges connect (0,0)–(5,0)–(10,0) and (5,-5)–(5,0)–(5,5).

---

*This module is part of a larger Game Server project – a multiplayer online game where players control dogs, collect loot, and compete on procedurally generated maps.*
