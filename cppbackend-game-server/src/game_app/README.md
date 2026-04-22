# Application Layer for Game Server

The `/src/game_app/` directory contains the application‑level logic that manages players, game state persistence, automatic saving, scoring, and the central game loop coordination for the Game Server.

## Code Description

- **Application** (`application.h`) – Central orchestrator that ties together the game model, player management, auto‑save, score recording, and the game clock. Handles player addition, game ticking, state saving/loading, and database integration for player scores. Connects player retirement signals to score persistence.
- **AutoSaveManager** (`auto_save_manager.h`) – Periodically saves the entire game state (game model + players) to a file. Uses a tick‑based accumulator and triggers a save when the configured period elapses. Disabled when period is zero or filename is empty.
- **GameClock** (`game_clock.h`) – Simple monotonic clock that tracks the total elapsed game time in milliseconds. Used for player join timestamps and play duration calculations.
- **GameStatePersistence** (`game_state_persistence.h`) – Handles saving and loading of the complete game state (game model + players) using Boost.Serialization with text archives. Performs thorough error handling, logging, and archive exception classification. Skips loading if the state file does not exist.
- **PlayerScoreRecorder** (`player_score_recorder.h`) – Records a player’s final score and play time into the database when a player retires. Provides query methods to retrieve top scores. Uses the database abstraction layer (`DatabaseInterface`).
- **Players** (`players.h` / `players.cpp`) – Manages all active players: registration, token generation, lookups by token/id/map/session. Handles restoration of players from saved state. Emits a `PlayerRetiredSignal` when a real player (not a bot) is removed due to dog idle timeout, allowing external components to record scores. Ensures exactly one connection per game session to the dog‑deleted signal.
- **Player** (`players.h`) – Represents a connected human player. Stores player ID, name, associated game session, join time, and a pointer to the in‑game dog. Forwards movement commands to the dog.
- **Token** (`token.h`) – Strong typedef (`Tagged<std::string>`) for player authentication tokens. Includes a generator that creates 32‑character hex tokens using two independent 64‑bit Mersenne Twister RNGs. Provides a validation function to check token format.

## Patterns Used

- **RAII** – Automatic cleanup of file streams, archive objects, and signal connections (`GameStatePersistence`, `Players` session connections).
- **Factory** – `Players::AddPlayer()` and `Players::AddRestoredPlayer()` create `Player` objects and their associated dogs.
- **Observer (Signal/Slot)** – Boost.Signals2 is used extensively: `Application` provides a save signal for tests; `Players` emits a `PlayerRetiredSignal`; `GameSession`’s dog‑deleted signal is connected to player removal.
- **Strategy (implicit)** – `AutoSaveManager` can be enabled/disabled by setting period/filename, swapping the persistence strategy.
- **Builder** – `GameStatePersistence::Load()` restores the entire game state step by step using `GameRepr`.
- **Tagged Type (Strong Typedef)** – `Token` is a `Tagged<std::string>` to avoid mixing with ordinary strings.
- **Singleton (implicit)** – The global logging core (`boost_logger`) is used via free functions.

## Libraries Used

- Boost.Signals2 – Signal/slot connections for player retirement, dog deletion, and test notifications.
- Boost.Serialization – Text archive serialization for game state persistence (`GameStatePersistence`).
- Boost.Asio – `io_context` used for timer strands and asynchronous operations (passed to `GameSession` and `Ticker`).
- Boost.Log – Referenced for logging errors and info messages.
- C++17 / C++20 STL – `<chrono>`, `<random>`, `<filesystem>`, `<unordered_map>`, `<map>`, `<memory>`, `<ranges>` (views).
- PostgreSQL (libpqxx) – Indirectly used via `db::DatabaseInterface` and `PlayerScoreRecorder`.

## Files Summary

| File | Purpose |
|------|---------|
| `application.h` | Main application class. Orchestrates game ticking, player addition, auto‑save, score recording, and database integration. Provides save/load methods and a testable save signal. |
| `auto_save_manager.h` | Tick‑driven periodic saver. Accumulates delta time and calls `GameStatePersistence::Save` when the period is reached. |
| `game_clock.h` | Simple game time abstraction. Tracks total elapsed milliseconds and allows advancing time. |
| `game_state_persistence.h` | Static methods `Save` and `Load` using Boost.TextArchive. Handles errors, missing files, and archive version mismatches. |
| `player_score_recorder.h` | Records player scores to the database on retirement. Provides `GetTopScores` for leaderboards. |
| `players.h` / `players.cpp` | Player container and management. Token generation, lookup by token/id/map/session, restoration from saved state, and retirement signal emission. |
| `token.h` | `Token` strong type with hex string validation and a cryptographically‑inspired generator using two 64‑bit RNGs. |

## Extra Data

### Environment Variables
- `GAME_DB_URL` – PostgreSQL connection string (read elsewhere, e.g., in `main_utils.h`), used by `DatabaseInterface` implementations.

### Integration with Main Server
Typical usage in the main server executable:

```cpp
#include "app/application.h"
#include "common/cmd_parser.h"
#include "common/json_loader.h"
#include "common/boost_logger.h"
#include "game_db/pooled_database.h"

auto args = parse::ParseCommandLine(argc, argv);
boost_logger::SendBoostLogToStream();
auto game = json_loader::LoadGame(args->config_file);
db::PooledDatabase db(args->db_url);
boost::asio::io_context ioc;

app::Application app(*game, *args, ioc, db);

// Tick loop (usually driven by a Ticker)
app.Tick(50ms);

// Add a player
const auto& player = app.AddPlayer("Alice", "map_1");
const Token* token = app.GetPlayers().FindTokenByPlayer(player);
```

### Auto‑Save Example
Auto‑save is configured via command‑line arguments (`--save-state-period` and `--state-file`):

```cpp
// In Application constructor:
auto_save_manager_(game_, players_,
                   std::chrono::milliseconds(cmd_args_.save_state_period),
                   cmd_args_.state_file)
```

### Player Retirement Signal Flow
When a dog remains idle for too long, the `GameSession` emits a dog‑deleted signal. `Players` connects to this signal, emits the `PlayerRetiredSignal` (allowing score recording), then removes the player from all containers.

```cpp
players_.OnPlayerRetired([](uint32_t dog_id, loot::Score score, const Player& player) {
    // Record final score and play duration
});
```

### Token Generation Example
Tokens are 32‑character hex strings, generated automatically when a player joins:

```cpp
Token token = token_gen_();   // e.g., "a1b2c3d4e5f6789012345678abcdef0123456789abcdef0123456789abcdef"
bool valid = IsTokenValid(*token);  // true
```

---

*This module is part of a larger Game Server project – a multiplayer online game where players control dogs, collect loot, and compete on procedurally generated maps.*
