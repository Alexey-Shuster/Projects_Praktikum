# Game Model Module

The `/src/game_model/` directory contains the core game domain model: dogs, maps, roads, buildings, offices, loot, game sessions, collision detection, and the main game loop logic. This module defines how players (dogs) move on maps, collect loot, interact with offices and how game state evolves over time.

## Code Description

- **Dog** (`dog.cpp/h`) – Represents a player’s dog. Stores position, speed, direction, bag of collected loot, score, idle time. Handles movement with road‑constrained physics: dogs move along roads, cannot leave them; speed is derived from map default speed and direction.
- **Game extra data** (`game_extra_data.h`) – Holds configuration not part of the basic map: per‑map dog speeds, bag capacities, loot generator settings (period, probability), loot type definitions (name, value, color, scale, etc.), and dog retirement timeout.
- **Game map** (`game_map.h`) – Defines `Map`, `Building`, `Office`, and `Road` (roads are horizontal/vertical segments). Maps contain roads, buildings, offices. Provides `GetRandomPoint()` for loot generation and office placement. Uses `RoadEngine` for road lookup and movement constraints.
- **Game model** (`game_model.cpp/h`) – The `Game` class aggregates all maps and game sessions. It creates or finds game sessions for a given map, updates all sessions, and manages session IDs. Also holds the loot generator and extra data.
- **Game session** (`game_session.cpp/h`) – A running instance of a map with active dogs (players) and bots. Contains the `Dog` container, `LootStorage`, and a `BotManager`. Updates game state each tick: moves dogs/bots, generates new loot, processes collisions (loot pickup, office delivery). Also handles dog retirement due to idle timeout.
- **Loot storage** (`loot_storage.cpp/h`) – Manages loot objects on a map. Generates new loot at random positions on roads, using configurable loot types. Stores loot objects in a map keyed by ID. Supports removal, lookup, and clearing.
- **Road** (`road.h`) – Simple horizontal or vertical road segment defined by start and end points. Used by `RoadEngine` for movement constraints.

## Patterns Used

- **Domain Model** – Core business logic encapsulated in `Dog`, `Map`, `GameSession`, etc., with clear invariants (dogs stay on roads).
- **Factory** – `Game::RequestGameSession()` creates new sessions on demand.
- **Command** – `Dog::SetDirection()` converts direction to a speed vector; `Dog::Move()` applies movement over time.
- **Observer / Signal** – `GameSession::DogDeletedSignal` (Boost.Signals2) notifies when a dog is retired.
- **Strategy** – `LootGenerator` is injected into `GameSession`; different generation strategies can be used.
- **Snapshot Isolation** – `GameSession` updates state based on a time delta; all changes are applied atomically per tick.
- **RAII** – No explicit resource management shown, but `LootStorage` and `BotManager` manage internal containers automatically.
- **Tagged Type** – `Map::Id`, `GameSession::Id`, `Office::Id` use `util::Tagged` for strong typing.

## Libraries Used

- **Boost.Signals2** – For the dog deletion signal (`game_session.h`).
- **Boost.Asio** – `io_context` and `strand` for thread‑safe session updates (`game_session.h`).
- **C++17 / C++20 STL** – `std::map`, `std::vector`, `std::unordered_map`, `std::random_device`, `std::mt19937`, `<chrono>`, `<ranges>`, `<algorithm>`.
- **Project‑internal** – `common/utils.h`, `common/tagged.h`, `common/game_utils/collision_detector.h`, `common/game_utils/loot_generator.h`, `game_bots/bot_manager.h`.

## Files Summary

| File | Purpose |
|------|---------|
| `dog.cpp/h` | Dog entity: position, speed, direction, bag, score, idle time, movement logic constrained by roads. |
| `game_extra_data.h` | Configuration container: per‑map speeds/bag capacities, loot types, loot generator parameters, dog retirement timeout. |
| `game_map.h` | Map definition with roads, buildings, offices. Provides road engine, random point generation, default speed/capacity. |
| `game_model.cpp/h` | Game aggregate: maps, sessions, session creation, session lookup, global update. |
| `game_session.cpp/h` | Game session (a running map instance): dogs, bots, loot storage, collision processing, dog retirement, state update per tick. |
| `loot_storage.cpp/h` | Loot object container: generation, removal, lookup. Uses random positions on roads. |
| `road.h` | Simple horizontal/vertical road segment. |

## Extra Data

### Environment Variables
None directly. Configuration is loaded from JSON (via `json_loader` in the common module) into `GameExtraData` and passed to the game model.

### Integration with Main Server
Typical usage in the game server:

```cpp
#include "model/game_model.h"
#include "common/json_loader.h"

auto extra_data = std::make_shared<extra_data::GameExtraData>();
auto game = std::make_shared<model::Game>(extra_data);
json_loader::LoadGame(config_file, *game); // populates maps and extra data

// Later, on player join:
auto [session_id, created] = game->RequestGameSession(map_id, ioc);
auto session = game->FindGameSession(session_id);
auto dog = session->RequestDog(player_id, player_name);
```

### Game Loop Integration
The ticker from the common module calls `Game::UpdateAllGameSessions()`:

```cpp
auto ticker = std::make_shared<tick::Ticker>(strand, tick_period, [&game](auto delta) {
    game->UpdateAllGameSessions(delta);
});
ticker->Start();
```

### Collision Processing
Each game session processes collisions in the following order:
1. Move all dogs and bots.
2. Generate new loot (if needed).
3. Detect collisions between moving dogs/bots and loot items / offices.
   - Loot pickup: if dog’s bag has capacity, loot is marked collected and added to bag.
   - Office delivery: all loot in bag is converted to score, loot objects removed from world.
4. Retire dogs that have been idle longer than the configured timeout (if retirement enabled).

### Dog Retirement
When a dog stands still (speed == 0) for more than `dog_retirement_time_` seconds, it is removed from the session. Its bag’s loot items are dropped back onto the map at the dog’s last position (marked not collected). A `DogDeletedSignal` is emitted with the dog’s ID and final score.

### Loot Generation
Loot is generated at random positions on roads. Each loot object has a type from `GameExtraData::GetLootTypes()`, which defines its score value, visual appearance, etc. The number of loot objects generated per tick is determined by `LootGenerator` based on time passed, current loot count, and number of dogs/bots.

---

*This module is part of a larger Game Server project – a multiplayer online game where players control dogs, collect loot, and compete on procedurally generated maps.*
