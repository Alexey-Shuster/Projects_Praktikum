# Game Utils (Collision Detection, Geometry, and Loot Generation) for Game Server

The `/src/common/game_utils/` directory provides core gameplay mechanics: geometric types for model and application coordinates, collision detection between moving gatherers and stationary items, and a probabilistic loot generator that maintains a target number of loot items on the map.

## Code Description

- **Geometry** (`geometry.h`) – Defines two separate coordinate systems:
  - `model_geom` – Integer-based coordinates (`Point2D`, `Size2D`, `Rectangle2D`) for the logical game model (grid cells, building positions, road segments).
  - `app_geom` – Floating‑point coordinates (`Position2D`, `Vec2D`, `Speed2D`, `Direction2D`) for physics, movement, and collision detection. Provides vector arithmetic, hashing for positions, and direction enum.
- **Collision Detection** (`collision_detector.cpp/h`) – Implements point‑segment distance calculation (`TryCollectPoint`) to detect when a moving gatherer (dog/player) passes close enough to collect an item. The `ItemGathererProvider` interface allows abstract access to gatherers and items, while `ItemGatherer` is a concrete vector‑based implementation. `FindGatherEvents` computes all collection events during a movement tick and returns them sorted by time.
- **Loot Generator** (`loot_generator.cpp/h`) – A probabilistic timer that controls item spawning. Given a base interval, probability, and current loot/looter counts, it determines how many new loot items should appear. The algorithm ensures that the total loot count does not exceed the number of looters, using a formula based on time without loot and a random generator.

## Patterns Used

- **Tagged / Namespace Separation** – Two distinct geometry namespaces (`model_geom` vs `app_geom`) prevent accidental mixing of integer grid coordinates and floating‑point world positions.
- **Strategy / Dependency Injection** – `LootGenerator` accepts a `RandomGenerator` functor, allowing deterministic (default) or truly random behaviour for testing/production.
- **Provider Interface** – `ItemGathererProvider` abstracts the source of gatherers and items, enabling the collision detector to work with different data structures (e.g., in‑memory game state or a snapshot).
- **Value Object** – Geometry structs (`Point2D`, `Position2D`, `Speed2D`) are immutable‑by‑design with default `<=>` comparisons and convenience methods like `ToString()`.
- **Algorithmic Function** – `TryCollectPoint` implements pure geometry logic (projection, distance) without side effects; `FindGatherEvents` orchestrates the full detection loop.
- **Builder (implicit)** – `ItemGatherer` provides `AddItem`/`AddGatherer` methods to construct a test fixture incrementally.

## Libraries Used

- C++17 / C++20 STL – `<algorithm>`, `<cmath>`, `<chrono>`, `<functional>`, `<vector>`, `<compare>`, `<iomanip>`, `<sstream>`, `<string>`
- No external dependencies beyond the standard library – the module is lightweight and portable.

## Files Summary

| File | Purpose |
|------|---------|
| `geometry.h` | Defines two coordinate systems: `model_geom` (integer, grid‑based) and `app_geom` (floating‑point, physics‑based). Includes `Vec2D`, `Position2D`, `Speed2D`, `Direction2D`, and conversion helpers. |
| `collision_detector.h` | Declares `CollectionResult`, `Item`, `Gatherer`, `ItemGathererProvider` interface, `ItemGatherer` concrete class, `FindGatherEvents` and sorting utilities. |
| `collision_detector.cpp` | Implements `TryCollectPoint` (point‑segment distance) and `FindGatherEvents` (brute‑force O(G*I) detection with time sorting). |
| `loot_generator.h` | Declares `LootGenerator` class with configurable base interval, probability, and random generator. |
| `loot_generator.cpp` | Implements the loot generation logic: computes shortage, probability over elapsed time, and returns the number of new items to spawn. |

---

*This module is part of a larger Game Server project – a multiplayer online game where players control dogs, collect loot, and compete on procedurally generated maps.*
```
