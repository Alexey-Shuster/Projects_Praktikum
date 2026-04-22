# Game Server Project Overview

The Game Server is a multiplayer online game where players control dogs, collect loot, and compete on procedurally generated maps. The server is written in modern C++ (C++17/20) and uses Boost libraries for networking, logging, serialization and JSON processing. It supports persistent game state (save/restore), PostgreSQL database for leaderboards, bot AI opponents and a full HTTP/1.1 API for client interaction.

## Architecture Overview

The server follows a layered architecture:

```
┌─────────────────────────────────────────────────────────────┐
│                HTTP Client (Browser / Mobile)               │
└─────────────────────────────────────────────────────────────┘
                              │ HTTP/1.1 (JSON)
                              ▼
┌─────────────────────────────────────────────────────────────┐
│  HTTP Server & API Module (http_server/)                    │
│  - Async request handling (Boost.Beast)                     │
│  - Trie‑based routing with path parameters                  │
│  - Static file serving, logging decorator                   │
└─────────────────────────────────────────────────────────────┘
                              │
                              ▼
┌─────────────────────────────────────────────────────────────┐
│  Application Layer (game_app/)                              │
│  - Coordinates players, sessions, scoring                   │
│  - Manages automatic saving (periodic / on shutdown)        │
│  - Delegates game logic to model and bots                   │
└─────────────────────────────────────────────────────────────┘
                              │
              ┌───────────────┼───────────────┐
              ▼               ▼               ▼
┌─────────────────┐ ┌─────────────────┐ ┌─────────────────┐
│  Game Model     │ │  Bot AI         │ │  Database       │
│  (game_model/)  │ │  (game_bots/)   │ │  (game_db/)     │
│  - Dogs, maps   │ │  - Pathfinding  │ │  - PostgreSQL   │
│  - Loot, shops  │ │  - Autonomous   │ │  - Leaderboard  │
│  - Collisions   │ │    behavior     │ │  - Mock/Local   │
│  - Game loop    │ │                 │ │    for testing  │
└─────────────────┘ └─────────────────┘ └─────────────────┘
                              │
                              ▼
┌──────────────────────────────────────────────────────────────┐
│  Common Infrastructure (common/) & Serialization (game_repr/)│
│  - Logging, config, tagged types, geometry, ticker           │
│  - Save/restore via Boost.Serialization                      │
└──────────────────────────────────────────────────────────────┘
```

### Data Flow

1. **Startup** – `main.cpp` parses command line, initialises logging, loads game configuration (JSON), sets up the database connection pool, and creates the `Application` layer.
2. **HTTP Request** – A client request arrives. The `LoggingRequestHandler` logs it, then `RequestHandler` dispatches to either API routing or static file serving.
3. **API Processing** – `ApiRouter` matches the path, validates method/auth/content‑type, and invokes the corresponding handler (e.g., `HandleGameState`). Handlers call into `Application` or directly into the game model.
4. **Game Logic** – The model updates dogs’ positions, collects loot, delivers to offices, and updates scores. Bots run their AI independently. The ticker (from `common/ticker.h`) triggers periodic updates.
5. **Persistence** – Game state can be saved to a file (Boost.Serialization) at shutdown or periodically. Scores are written to PostgreSQL (or a mock/local database for testing).
6. **Response** – API handlers serialise results to JSON (using `serialize_api`) and send HTTP responses.

## Code Description (by Module)

### Common Module (`/src/common/`)
Shared infrastructure: logging (Boost.Log), command‑line parsing (Boost.Program_Options), constants, JSON game loader, strong typedefs (`tagged.h`), geometry helpers, HTTP utilities, and a generic ticker for time‑driven updates.

### Application Layer (`/src/game_app/`)
Manages players (`Players` class with token authentication), game sessions, and the central `Application` facade. Handles player actions (join, move), ticks (game loop), automatic saving (periodic or on shutdown), and scoring (retrieves leaderboard from database).

### Bot AI Module (`/src/game_bots/`)
Implements computer‑controlled dogs using a behaviour tree / state machine. Bots roam road networks, use pathfinding (A* on road graph) to navigate, collect loot, and deliver it to offices. They respawn after delivering loot.

### Database Persistence Layer (`/src/game_db/`)
Abstracts database access with `DatabaseInterface`. Provides `PooledDatabase` (PostgreSQL with connection pool), `LocalDatabase` (SQLite for development), and `MockDatabase` (in‑memory for testing). Includes repository interfaces, unit of work (transaction) support, and schema migration runner.

### Game Model Module (`/src/game_model/`)
Core domain: `Map`, `Road`, `Building`, `Office`, `Loot`, `Dog` (player or bot), `GameSession` (map instance with loot generator and dogs), collision detection, movement logic, loot collection, and office delivery. Also contains the loot generator and game parameters.

### Game State Serialization Module (`/src/game_repr/`)
Representation classes (`GameRepr`, `GameSessionRepr`, `DogRepr`, `LootStorageRepr`, `PlayersRepr`) that serialise the entire game state using Boost.Serialization (text archives). Used for save/restore across server restarts.

### HTTP Server and API Module (`/src/http_server/`)
Full HTTP server based on Boost.Beast. Contains:
- `http_server.h/cpp` – async listener and session classes.
- `request_handler.h/cpp` – dispatches to API or static files.
- `logging_request_handler.h` – decorator for request/response logging.
- `api_router.h/cpp` – trie‑based router with path parameters.
- `api_handler.h/cpp` – implements all game API endpoints.
- `http_response.h/cpp` – fluent response builder.
- `serialize_api.h/cpp` – converts domain objects to Boost.JSON.

## Patterns Used Across the Project

- **RAII** – Automatic resource management: file handles, database connections, log sinks, timers.
- **Factory** – `json_loader::LoadGame()` builds the `model::Game` from JSON; `*Repr` classes rebuild domain objects during restore.
- **Builder** – `response::Builder` for HTTP responses; step‑by‑step construction of game objects from JSON.
- **Decorator** – `LoggingRequestHandler` adds logging without modifying the original handler.
- **Strategy** – Swappable log formatters (plain text vs JSON); different database backends (PostgreSQL / SQLite / mock).
- **Tagged Type (Strong Typedef)** – `Tagged<Value, Tag>` prevents accidental mixing of IDs (e.g., `Map::Id` vs `Office::Id`).
- **Strand‑based Asynchronous Execution** – `ticker.h` and `RequestHandler` use `boost::asio::strand` for thread‑safe callback dispatch.
- **Memento / Serialization Proxy** – `*Repr` classes act as serializable snapshots of domain objects.
- **Chain of Responsibility** – Request handling passes through logging → dispatch → routing → handler.
- **Singleton (implicit)** – Boost.Log core accessed via static methods; database connection pool shared across repositories.
- **Unit of Work** – Database transactions encapsulate multiple repository operations.

## Libraries Used

- **Boost.Log** – Structured logging with severity levels, attributes, file/console sinks.
- **Boost.Program_Options** – Command‑line argument parsing.
- **Boost.Asio** – I/O context, strands, timers, signal handling.
- **Boost.Beast** – HTTP protocol, websockets (if used), async read/write.
- **Boost.JSON** – Parsing and serialisation for API and configuration.
- **Boost.Serialization** – Game state save/restore (text archives).
- **Boost.Date_Time** – Timestamp formatting for logs.
- **Boost.Filesystem** (via C++17 `std::filesystem`) – Path manipulation.
- **PostgreSQL (libpqxx)** – Database access for leaderboards.
- **SQLite3** – Optional local database (for development).
- **C++17 / C++20 STL** – `std::filesystem`, `std::chrono`, `std::random`, `std::unordered_map`, `std::optional`, `std::variant`, `std::ranges`, `std::views`, smart pointers.

## Directory Structure

```
/src/
├── common/           – Shared infrastructure (logging, config, utils, ticker)
├── game_app/         – Application facade, players, automatic saving
├── game_bots/        – AI for computer‑controlled dogs
├── game_db/          – Database abstraction (PostgreSQL, SQLite, mock)
├── game_model/       – Core domain model (dogs, maps, loot, sessions)
├── game_repr/        – Boost.Serialization representations for save/restore
└── http_server/      – HTTP server, API routing, request handling, serialization
```

## Files Summary (High‑Level)

| Module | Key Files | Purpose |
|--------|-----------|---------|
| **common** | `boost_logger.cpp/h`, `cmd_parser.h`, `constants.h`, `json_loader.cpp/h`, `tagged.h`, `ticker.h`, `utils.cpp/h` | Logging, config, JSON loading, strong types, timers, helpers |
| **game_app** | `application.cpp/h`, `players.cpp/h`, `token.h`, `auto_saver.h` | App facade, player management, tokens, periodic saving |
| **game_bots** | `bot.cpp/h`, `bot_ai.cpp/h`, `pathfinder.cpp/h` | AI logic, pathfinding, bot behaviour |
| **game_db** | `database_interface.h`, `pooled_database.cpp/h`, `mock_database.h`, `local_database.h`, `repositories.h`, `migrations.cpp` | DB abstraction, connection pool, repositories |
| **game_model** | `game_model.h`, `game_session.cpp/h`, `dog.cpp/h`, `map.cpp/h`, `loot_storage.cpp/h`, `collision.cpp/h` | Domain logic, game loop, loot generation |
| **game_repr** | `game_repr.h`, `game_session_repr.h`, `loot_repr.h`, `players_repr.h` | Save/restore state via Boost.Serialization |
| **http_server** | `http_server.cpp/h`, `request_handler.cpp/h`, `api_handler.cpp/h`, `api_router.cpp/h`, `http_response.cpp/h`, `serialize_api.cpp/h`, `logging_request_handler.h` | Async HTTP server, API endpoints, routing, JSON serialisation |

## Extra Data

### Command‑Line Arguments (from `cmd_parser.h`)

| Argument | Type | Default | Description |
|----------|------|---------|-------------|
| `--tick-period` | int | `0` (auto disabled) | Milliseconds between automatic game ticks |
| `--config-file` | string | (required) | Path to game configuration JSON |
| `--www-root` | string | `./static` | Root directory for static files |
| `--state-file` | string | empty | File to save/load game state (Boost.Serialization) |
| `--no-database` | flag | false | Use mock database (skip PostgreSQL) |
| `--local-database` | flag | false | Use SQLite local database instead of PostgreSQL |

### Environment Variables

- `GAME_DB_URL` – PostgreSQL connection string (e.g., `postgresql://user:pass@localhost/game`). Used when `--no-database` is not set and `--local-database` is false.

### Building and Running

The server is built with CMake (C++17 or C++20). Dependencies: Boost (log, program_options, asio, beast, json, serialization, date_time), libpqxx, SQLite3 (optional). Example build:

```bash
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)
```

Run the server:

```bash
./game_server --config-file ../config/game.json --www-root ../static --tick-period 50
```

### Integration Example (from `main.cpp`)

```cpp
// Parse command line
auto args = parse::ParseCommandLine(argc, argv);
// Initialise logging
boost_logger::InitBoostLogSetFilter();
boost_logger::SendBoostLogToStream();
// Load game configuration
auto game_data = json_loader::LoadGame(args->config_file);
// Create database (real, local, or mock)
std::unique_ptr<db::DatabaseInterface> database = ...;
// Create application
app::Application app(game_data, *args, ioc, *database);
// Load saved state if provided
if (!args->state_file.empty()) app.LoadGameState(args->state_file);
// Start HTTP server
http_server::ServeHttp(ioc, {address, port}, logging_handler);
// Run io_context with worker threads
utils::RunWorkers(num_threads, [&ioc] { ioc.run(); });
// Save state on shutdown
if (!args->state_file.empty()) app.SaveGameState(args->state_file);
```

### Logging Example

All modules use structured JSON logging (via `boost_logger`). Sample output:

```json
{
  "timestamp": "2025-01-15T12:34:56.789",
  "data": { "port": 8080, "address": "0.0.0.0" },
  "message": "Server has started"
}
```

### Saving / Restoring Game State

Game state (sessions, dogs, loot, players) is saved using `GameRepr` and Boost.TextArchive. The save file is portable across server restarts and platform changes. Restore occurs automatically if `--state-file` points to an existing file.

---

*This project is a complete, production‑ready game server for a multiplayer online game where players control dogs, collect loot, and compete on procedurally generated maps. The modular architecture allows easy extension (e.g., new AI behaviours, different database backends, additional API endpoints).*
