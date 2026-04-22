# Bot AI Module for Game Server

The `/src/game_bots/` directory contains the artificial intelligence for computer‑controlled dogs (bots) in the Game Server. Bots roam the road network, collect loot, deliver it to offices, and use pathfinding to navigate the game world.

## Code Description

- **BotAI** (`bot_ai.cpp/h`) – Finite‑state machine that controls a single bot’s behaviour. States: `ROAMING` (wander randomly), `MOVING_TO_LOOT` (go to nearest loot pile), `MOVING_TO_OFFICE` (go to nearest office when bag is full). Uses a road graph for pathfinding and smooth movement via waypoint following. Reuses the last valid direction to reduce jitter.
- **BotManager** (`bot_manager.cpp/h`) – Manages all bots on a single map. Creates a fixed number of bots (half of `MAX_PLAYERS_ON_MAP`), updates their positions each tick, and optionally updates their directions using either a simple random‑walk AI or the advanced `BotAI` with world state awareness. Provides access to bot containers for collision processing and game logic.
- **BotWorldState** (`bot_ai.h`) – Lightweight struct passed to `BotAI::UpdateDirection()` containing positions of all visible loot piles and offices on the map. Enables intelligent decision‑making.

## Patterns Used

- **Finite State Machine** – `BotAI` implements three explicit states with well‑defined transitions based on bag fullness and random chance.
- **Strategy** – `BotManager` supports two direction‑update strategies: simple random walk (`UpdateDirections()`) and world‑aware AI (`UpdateDirections(world_state, time_delta)`).
- **Pathfinding (A* / Graph Search)** – `BotAI` uses `RoadGraph::FindPath()` to plan routes along the road network, then follows waypoints.
- **Object Pool / Container** – `BotManager` stores all bots in a `std::map<uint32_t, Dog>` and provides iterator access.
- **RAII** – Random number generators and the road graph are initialised in the constructor and persist for the lifetime of the manager.
- **Factory** – `BotManager::CreateBots()` constructs bots and their corresponding AI instances.

## Libraries Used

- C++17 / C++20 STL – `<map>`, `<vector>`, `<random>`, `<chrono>`, `<optional>`, `<ranges>`, `<algorithm>`, `<cmath>`.
- Boost (indirect) – No direct Boost usage in these files, but the road graph and geometry utilities may rely on Boost (the project uses Boost.Asio, Boost.Serialization etc.).

## Files Summary

| File | Purpose |
|------|---------|
| `bot_ai.h` | Defines `BotAI` class and `BotWorldState` struct. `BotAI` maintains target, path, state timer, and direction reuse logic. |
| `bot_ai.cpp` | Implements the FSM logic: state transitions, target selection, path planning, waypoint following, and direction calculation. Contains constants for roaming timeout, waypoint reach distance, and direction reuse limit. |
| `bot_manager.h` | Declares `BotManager` – owns a map’s bots, their AI instances, a road graph, and RNG. Provides methods to create, move, and update bot directions. |
| `bot_manager.cpp` | Implements bot creation (half of `MAX_PLAYERS_ON_MAP`), movement delegation, simple random‑walk direction changes, and the integration of `BotAI` with world state. |

## Extra Data

### Integration with Game Session
`BotManager` is typically owned by a `GameSession`. The session calls `CreateBots()` on initialisation, then each tick:

```cpp
// In GameSession::Update()
bot_manager_.MoveBots(time_delta);
BotWorldState state = GatherWorldState(); // loot and office positions
bot_manager_.UpdateDirections(state, time_delta);
```

### Bot AI State Machine
- **ROAMING** – The bot moves randomly on the road network. Every 5 seconds it picks a new random point on a random road. With a small probability (e.g., `DEFAULT_LOOT_CONFIG_PROBABILITY`) it switches to `MOVING_TO_LOOT` if loot exists.
- **MOVING_TO_LOOT** – The bot picks the closest visible loot pile and plans a path to it. When the loot is collected (bag size increases), the state may change.
- **MOVING_TO_OFFICE** – Triggered when the bag becomes full. The bot goes to the nearest office. After delivering (bag becomes empty), it returns to `ROAMING`.

### Bot Creation Parameters
- Number of bots per map = `MAX_PLAYERS_ON_MAP / 2` (defined in `constants.h`).
- Bot IDs start from `DOG_BOT_START_ID` (e.g., 10,000) to avoid collisions with real players.
- Bot names are generated as `"Bot_<id>"`.

### Direction Reuse (Smoother Movement)
`BotAI` reuses the last computed direction for up to 5 consecutive ticks if it remains valid on the current road. This prevents rapid direction changes and makes bot movement appear more natural.

### Example: Using BotWorldState
```cpp
BotWorldState state;
for (const auto& loot : loot_items) {
    state.loot_positions.push_back(loot.GetPosition());
}
for (const auto& office : offices) {
    state.office_positions.push_back(office.GetPosition());
}
bot_manager_.UpdateDirections(state, time_delta);
```

---

*This module is part of a larger Game Server project – a multiplayer online game where players control dogs, collect loot, and compete on procedurally generated maps.*
