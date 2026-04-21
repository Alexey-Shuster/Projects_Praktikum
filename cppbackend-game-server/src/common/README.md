# Common Module for Game Server

The `/src/common/` directory contains shared infrastructure code used across the Game Server project: logging, configuration loading, JSON utilities, strong type definitions, geometry helpers, HTTP helpers, and a generic ticker for time‑driven updates.

## Code Description

- **Logging** (`boost_logger.cpp/h`) – Wraps Boost.Log to produce structured JSON logs with custom attributes (timestamp, severity, additional data). Supports console and file sinks with rotation.
- **Command‑line parsing** (`cmd_parser.h`) – Uses Boost.Program_Options to parse arguments like `--tick-period`, `--config-file`, `--www-root`, and various boolean flags.
- **Constants** (`constants.h`) – Centralises numeric constants, JSON field names, HTTP content types, API paths, error codes, and game logic parameters.
- **JSON game loader** (`json_loader.cpp/h`) – Loads the game configuration from a JSON file (maps, roads, buildings, offices, loot types, loot generator settings) and constructs the domain model (`model::Game`).
- **Tagged types** (`tagged.h`) – Implements a type‑safe wrapper (`Tagged<Value, Tag>`) to avoid accidental mixing of semantically different values (e.g. `Office::Id` vs `Map::Id`).
- **Utilities** (`utils.cpp/h`) – Provides filesystem helpers (sub‑path verification), geometry calculations, direction↔string conversions, random number generation, URL decoding, MIME type detection, and HTTP header parsing.
- **Ticker** (`ticker.h`) – A timer that runs on a `boost::asio::strand` and invokes a user callback at fixed intervals, used for the game loop and state updates.
- **Main utilities** (`main_utils.h`) – Contains environment configuration (database URL), test database cleanup, worker thread management, and a portable pause function.

## Patterns Used

- **RAII** – Automatic resource management for file handles, log sinks, and timers (`json_loader`, `boost_logger`).
- **Factory** – `json_loader::LoadGame()` constructs the complete `model::Game` from a JSON configuration.
- **Strategy** – `MyFormatter` / `MyFormatterJSON` provide swappable log output formats (plain text vs JSON).
- **Tagged Type (Strong Typedef)** – `tagged.h` provides type‑safe wrappers (e.g. `Office::Id`, `Map::Id`) preventing implicit conversions.
- **Strand‑based Asynchronous Execution** – `ticker.h` uses `boost::asio::strand` for thread‑safe callback dispatch.
- **Builder** – Step‑by‑step construction of complex game objects from JSON (`LoadMaps()`, `LoadRoads()`, etc.).
- **Singleton (implicit)** – `boost::log::core` global logging core accessed via static methods.

## Libraries Used

- Boost.Log – Structured logging with severity levels, attributes, and sinks.
- Boost.Program_Options – Command‑line argument parsing.
- Boost.Asio – I/O context, strands, timers (used by `Ticker`).
- Boost.Beast – HTTP components (referenced in `utils.h` for request handling).
- Boost.JSON – JSON parsing and serialisation.
- Boost.Date_Time – Timestamp formatting for logs.
- C++17 / C++20 STL – Filesystem, chrono, random, unordered containers, smart pointers.
- PostgreSQL (libpqxx) – Indirectly used via environment helpers in `main_utils.h`.

## Files Summary

| File | Purpose |
|------|---------|
| `boost_logger.cpp/h` | Initialises Boost.Log, provides JSON and plain‑text formatters, and convenience logging functions for server events, requests, responses, errors, and debug. |
| `cmd_parser.h` | Defines the `Args` structure and `ParseCommandLine()` to process command‑line options and validate paths. |
| `constants.h` | Global constants: game parameters, JSON field names, HTTP content types, API endpoint strings, error codes, and messages. |
| `json_loader.cpp/h` | Loads the game configuration from a JSON file, parses maps, roads, buildings, offices, loot types, and loot generator settings. Includes diagnostic function `CheckGameLoad()`. |
| `main_utils.h` | Provides environment variable reading (`GAME_DB_URL`), test database cleanup, worker thread launcher (`RunWorkers`), and a console pause utility. |
| `sdk.h` | Minimal header to set `WIN32` SDK version (for Windows builds). |
| `tagged.h` | Implements `Tagged<Value, Tag>` – a generic strong typedef with equality and hashing support. |
| `ticker.h` | A `std::enable_shared_from_this` timer that runs on a `boost::asio::strand` and invokes a handler with the elapsed time delta. |
| `utils.cpp/h` | Miscellaneous helpers: filesystem (sub‑path check), geometry (distance, position conversion), direction conversions, random numbers, URL decoding, MIME type detection, and HTTP token extraction. |

## Extra Data

### Environment Variables
- `GAME_DB_URL` – PostgreSQL connection string for the game database (read in `main_utils.h`).

### Integration with Main Server
The common module is used by the main game server executable. Typical usage:

```cpp
#include "common/cmd_parser.h"
#include "common/json_loader.h"
#include "common/boost_logger.h"

auto args = parse::ParseCommandLine(argc, argv);
boost_logger::SendBoostLogToStream();
auto game = json_loader::LoadGame(args->config_file);
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

### Ticker Usage

```cpp
auto strand = net::make_strand(ioc);
auto ticker = std::make_shared<tick::Ticker>(strand, 50ms, [](auto delta) {
    game.Update(delta);
});
ticker->Start();
```

---

*This common module is part of a larger Game Server project – a multiplayer online game where players control dogs, collect loot, and compete on procedurally generated maps.*
```
