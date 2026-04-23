# Game Server Project

The Game Server is a multiplayer online game backend where players control dogs and collect loot.
Written in modern C++20, the server is designed for high concurrency (Asio thread pool), extensibility (plugin‑like libraries) and operational robustness (logging, state persistence).

It leverages Boost libraries (ASIO, Beast, Log, Program Options, Serialization, JSON etc.) for networking, logging and data handling, and uses PostgreSQL (via libpqxx) to store leaderboards and support persistent game state (save/restore).
Additional features include bot AI opponents and a REST API for client interaction.

The provided CMakeLists.txt is capable of building the project on both Windows and Linux, using either GCC or Clang as the C++ compiler.

---

## Architecture Overview

The project is split into several static libraries (see `CMakeLists.txt`), each responsible for a distinct layer of the application:

| Library | Purpose |
|---------|---------|
| **Common_Lib** | Shared infrastructure: logging, configuration, JSON loading, tagged types, geometry, random utilities, ticker, command-line parsing. |
| **Game_Bots_Lib** | AI bots that can join games and act as autonomous players. |
| **Game_Model_Lib** | Core domain model: `Game`, `Map`, `Road`, `Dog`, `Loot`, collision detection, loot generation, game session logic. |
| **Game_DB_Lib** | Database abstraction: connection pool, unit of work, repositories, migrations, mock/local/remote implementations. |
| **Game_App_Lib** | Application layer: player management, game clock, state persistence (save/load), auto‑save, score recording. |
| **Game_Repr_Lib** | Serialisation of game state into Archive (state dumps). |
| **Http_Server_Lib** | HTTP/1.1 server using Boost.Beast, request routing, API handlers, logging decorator. |

The main executable (`game_server`) ties everything together: parses command line, loads game configuration, sets up logging and database, starts the HTTP server, and runs the I/O context on a thread pool.

---

## Code Description

### Common_Lib (src/common/)
- **Boost logging** – structured JSON logs with severity, timestamp, and custom fields; console and file sinks with rotation.
- **Command‑line parsing** – `--tick-period`, `--config-file`, `--www-root`, `--state-file`, `--no-database`, `--local-database`, `--randomize-state`, `--save-state-period`.
- **Constants** – centralised game parameters, JSON field names, HTTP content types, error codes.
- **JSON loader** – loads `config.json` (maps, roads, buildings, offices, loot types, generator settings) and builds the domain model.
- **Tagged types** – `Tagged<Value, Tag>` prevents accidental mixing of e.g. `Office::Id` and `Map::Id`.
- **Utilities** – filesystem helpers, geometry, direction↔string, random numbers, URL decoding, MIME types, HTTP header parsing.
- **Ticker** – timer on a `boost::asio::strand` that invokes a callback at fixed intervals (game loop).
- **Main utils** – environment helpers (`GAME_DB_URL`), test DB cleanup, worker thread launcher, portable pause.

### Game_Bots_Lib (src/game_bots/)
- `BotAI` – decision logic for bot movement and loot collection.
- `BotManager` – spawns and updates bots, synchronises them with game sessions.

### Game_Model_Lib (src/game_model/)
- **Game** – top-level container of maps and global settings.
- **GameSession** – a running instance of a map, holds dogs, loot, and the game clock.
- **Dog** – position, velocity, collected loot, bag.
- **Road**, **RoadEngine**, **RoadGraph** – map topology and pathfinding.
- **LootStorage** – manages loot items and respawning.
- **Collision detection** – dog–office and dog–loot collisions.
- **Loot generator** – configurable spawn logic (interval, probability, per‑player limits).

### Game_DB_Lib (src/game_db/)
- **DatabaseInterface** – abstract repository for players and game records.
- **ConnectionPool** – thread‑safe pool of `pqxx::connection` objects.
- **PooledDatabase** / **LocalDatabase** / **MockDatabase** – different backends (real PostgreSQL, SQLite‑based local, in‑memory dummy).
- **Unit of work** – transaction management for repositories.
- **Migrations** – schema versioning and upgrade scripts.

### Game_App_Lib (src/game_app/)
- **Application** – orchestrates game sessions, players, and persistence.
- **Players** – tracks active players, maps tokens to players.
- **Token** – type‑safe authentication token (UUID).
- **GameClock** – time keeping for the game loop.
- **GameStatePersistence** – save/load full game state to/from JSON.
- **AutoSaveManager** – periodic state saving.
- **PlayerScoreRecorder** – updates high scores and statistics.

### Game_Repr_Lib (src/game_repr/)
- Converts domain objects (`GameSession`, `Loot`, `Players`, etc.) into Archive using `boost::archive::text_oarchive`.
- Used by API handlers and state persistence.

### Http_Server_Lib (src/http_server/)
- `ServeHttp` – starts an HTTP/1.1 server on a given address/port using Boost.Beast.
- **RequestHandler** – core request dispatcher, routes to API or static files.
- **ApiRouter** – maps URL paths to handler functions (e.g. `/api/game/state`, `/api/game/join`, `/api/game/tick`).
- **ApiHandler** – implements game API endpoints (join, move, state, tick).
- **LoggingRequestHandler** – decorator that logs each request and response.
- **HttpResponse** – helper to build standard HTTP responses with JSON bodies.

### Main Executable (src/main.cpp)
Flow:
1. Parse command line (`parse::ParseCommandLine`).
2. Initialise Boost.Log (console + file).
3. Create `io_context` with `hardware_concurrency` threads.
4. Load game configuration (`json_loader::LoadGame`).
5. Choose database backend (mock, local, or pooled remote PostgreSQL).
6. Create `app::Application` (holds game, players, persistence).
7. Load previous game state if `--state-file` provided.
8. Set up signal handlers for graceful shutdown.
9. Start HTTP server on `0.0.0.0:8080` with logging wrapper.
10. Run worker threads (`utils::RunWorkers`).
11. On shutdown, save state if needed and log exit.

---

## Patterns Used

- **RAII** – file handles, log sinks, database connections, timers.
- **Factory** – `json_loader::LoadGame` constructs the complete `Game` object.
- **Strategy** – log formatters (plain text vs JSON); database backends (mock/local/remote).
- **Tagged Type (Strong Typedef)** – `tagged.h` provides type‑safe wrappers.
- **Strand‑based Asynchronous Execution** – `Ticker` and HTTP server use `boost::asio::strand` for thread safety.
- **Builder** – step‑by‑step building of game objects from JSON (maps, roads, buildings).
- **Repository / Unit of Work** – database access abstraction.
- **Decorator** – `LoggingRequestHandler` wraps the main request handler.
- **Dependency Injection** – `Application` receives `DatabaseInterface` and `Args` via constructor.
- **Observer (implicit)** – signal set for termination.

---

## Libraries Used

| Library | Purpose |
|---------|---------|
| **Boost** (1.78+) | Log, Program Options, Asio, Beast, JSON, Date_Time, Filesystem, Serialization |
| **libpqxx / libpq** | PostgreSQL client (connection pooling, queries) |
| **C++17/20 STL** | `std::filesystem`, `std::chrono`, `std::random`, `std::unordered_map`, smart pointers |
| **Catch2** (3.4) | Unit tests (game model, loot generator, collision detection, serialisation, database) |
| **Conan** (1.66) | Package management (dependencies + CMake integration) |
| **Docker** | Two‑stage build (gcc:11.3 for compilation, ubuntu:22.04 for runtime) |

---

## Files Summary (Key Files)

| Path | Purpose |
|------|---------|
| `src/main.cpp` | Entry point, orchestrates all libraries. |
| `src/common/cmd_parser.h` | Command‑line parsing (`Args` structure). |
| `src/common/boost_logger.cpp/h` | Boost.Log initialisation and convenience logging functions. |
| `src/common/json_loader.cpp/h` | Loads `config.json` into `model::Game`. |
| `src/common/tagged.h` | Type‑safe wrapper for IDs. |
| `src/common/ticker.h` | Periodic timer on `boost::asio::strand`. |
| `src/game_model/game_session.cpp/h` | Core game logic (update, tick, collision). |
| `src/game_model/game_model.cpp/h` | Game container, map management. |
| `src/game_db/pooled_database.h` | PostgreSQL connection pool and repository. |
| `src/game_db/mock_database.h` | In‑memory dummy DB for tests / `--no-database`. |
| `src/game_app/application.cpp/h` | Application facade, ties everything together. |
| `src/game_app/game_state_persistence.cpp/h` | Save/load game state to/from JSON. |
| `src/http_server/api_handler.cpp/h` | API endpoints (join, move, state, tick). |
| `src/http_server/request_handler.cpp/h` | Dispatches requests to API or static files. |
| `tests/*.cpp` | Unit tests for model, loot generator, collision detection, serialisation, database. |
| `CMakeLists.txt` | Build configuration (static libraries, executables, test targets). |
| `conanfile.txt` | Conan dependencies. |
| `Dockerfile` | Multi‑stage container build. |

---

## Extra Data

### Environment Variables
- `GAME_DB_URL` – PostgreSQL connection string (e.g. `postgresql://user:pass@localhost/game`).  
  Used in `main_utils.h` and passed to `ConnectionPool`.

### Command‑Line Arguments (from `parse::Args`)
| Argument | Type | Default | Description |
|----------|------|---------|-------------|
| `--help` / `-h` | - | - | Show help |
| `--tick-period` / `-t` | uint32 | required | Game tick interval in milliseconds. |
| `--config-file` / `-c` | string | required | Path to game configuration JSON. |
| `--www-root` / `-w` | string | required | Root directory for static files. |
| `--state-file` | string | `""` | File to load/save game state. |
| `--no-db` / `-n` | flag | `false` | Disable any DB, use `MockDatabase`. |
| `--lcl-db` / `-l` | flag | `false` | Use `LocalDatabase` (SQLite) instead of remote PG. |
| `--randomize-spawn-points` / `-r` | flag | `false` | Randomise dog positions. |
| `--bots` / `-b` | flag | `false` | Activate bots. |
| `--save-state-period` / `-s` | uint32 | `""` | Auto‑save interval (seconds). |

Example:
```bash
./game_server.exe -t 50 -c ../../data/config.json -w ../../static -b -n -r
```

### Docker Usage
Build the image:
```bash
docker build -t game_server .
```
Run with default arguments (tick 50 ms, config `/app/data/config.json`, static `/app/static`):
```bash
docker run -p 8080:8080 -e GAME_DB_URL="postgresql://..." game_server
```
Override command line:
```bash
docker run -p 8080:8080 game_server --tick-period 30 --no-database
```

### Logging Example
JSON log output (console or file):
```json
{
  "timestamp": "2025-01-15T12:34:56.789",
  "data": { "port": 8080, "address": "0.0.0.0" },
  "message": "Server has started"
}
```

### Game State Persistence
The server can save/load the entire game state (players, dogs, loot, scores) to an Archive file. This enables restarts without losing progress. The format is produced by `Game_Repr_Lib` and consumed by `GameStatePersistence`.

### Integration with Main Server (Typical Usage)
```cpp
#include "common/cmd_parser.h"
#include "common/json_loader.h"
#include "common/boost_logger.h"
#include "game_app/application.h"
#include "http_server/serve_http.h"

auto args = parse::ParseCommandLine(argc, argv).value();
boost_logger::InitBoostLogSetFilter();
auto game = json_loader::LoadGame(args.config_file);
auto database = create_database(args); // mock/local/pooled
app::Application app(game, args, ioc, *database);
http_server::ServeHttp(ioc, {address, port}, request_handler);
```

---

## Building and Testing

### Dependencies (via Conan)
- Boost (≥1.78.0)
- libpqxx (7.7.4)
- Catch2 (3.4.0)

### Build Steps
```bash
mkdir build && cd build
conan install .. --build=missing
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build . -- -j $(nproc)
```

### Running Tests
Tests are built only for `Debug` configurations:
```bash
cd build/bin
./game_model_tests
./loot_generator_tests
./collision_detection_tests
./state-serialization-tests
./database_tests_local
```

### Database Migrations
When using `PooledDatabase`, the server automatically runs `db::RunMigrations` on startup. Migrations are SQL scripts stored in `src/game_db/db_migrations.h/`.

---

## Licence
This project is licensed under the MIT License.
You are free to use, modify and distribute it as long as you include the original copyright notice.
```
===============================================================================
 Game Server v2.1
 Copyright (c) 2026 Alexey Shuster
 Licensed under the MIT License
===============================================================================
```
