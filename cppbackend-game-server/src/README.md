# Common Module

Shared infrastructure for the Game Server: logging, config parsing, JSON loading, strong types, geometry helpers, HTTP utilities, and a ticker.

## Key Components

| File | Purpose |
|------|---------|
| `boost_logger.cpp/h` | JSON/plain logging with Boost.Log |
| `cmd_parser.h` | Command‑line args (`--tick-period`, `--config-file`, etc.) |
| `constants.h` | Game constants, JSON field names, HTTP codes |
| `json_loader.cpp/h` | Load game maps, roads, buildings, loot from JSON |
| `tagged.h` | Type‑safe wrappers (`Tagged<Value, Tag>`) |
| `utils.cpp/h` | Path checks, geometry, direction conversions, MIME types |
| `ticker.h` | Timer on `boost::asio::strand` for game updates |
| `main_utils.h` | DB env vars, worker threads, cleanup helpers |

## Libraries Used

- Boost.Log, Boost.ProgramOptions, Boost.Asio, Boost.Beast, Boost.JSON, Boost.Date_Time
- C++17 STL (filesystem, chrono, random)

## Quick Example

```cpp
#include "common/cmd_parser.h"
#include "common/json_loader.h"
#include "common/boost_logger.h"

auto args = parse::ParseCommandLine(argc, argv);
boost_logger::SendBoostLogToStream();
auto game = json_loader::LoadGame(args->config_file);
```
