# Game State Serialization Module

The `/src/game_repr/` directory contains representation classes for saving and restoring the complete state of the game server: game sessions, dogs, loot storage, players, and their associations. It relies on **Boost.Serialization** to produce portable text archives.

## Code Description

- **GameRepr** – Top‑level representation of the whole game. Aggregates `GameSessionRepr` objects and a `PlayersRepr`. Provides `Restore()` to rebuild the `model::Game` and `app::Players` from saved data.
- **GameSessionRepr** – Represents a game session (map instance). Stores session ID, map ID, a list of `DogRepr` entries, and a `LootStorageRepr`. During restore, it reconstructs the session, restores loot storage (obtaining a loot ID → pointer map), then restores dogs using that map, and finally adds the session back into the game.
- **DogRepr** – Represents a single dog (player character). Stores position, speed, direction, score, bag contents (as loot IDs), and the index of the current road. Restores a `model::Dog` and re‑attaches loot pointers using a per‑session lookup map.
- **LootStorageRepr** – Represents the loot storage of a session. Saves the next available loot ID and a list of `LootObjectRepr` entries. Restores clears the storage, sets the next ID, and rebuilds loot objects, returning a map from loot ID to raw pointer for use by dog bag restoration.
- **LootObjectRepr** – Stores a loot object’s ID, position, loot type index (into the map’s loot types list), and collected flag.
- **PlayersRepr** – Represents the collection of players. Saves the next player ID and a list of `PlayerRepr`. Restores sets the next ID and adds each player back into the `app::Players` container, using a session lookup map.
- **PlayerRepr** – Stores player ID, name, session ID, and authentication token. Restores recreates the player and associates it with the correct game session.

## Patterns Used

- **Memento / Serialization Proxy** – Each `*Repr` class acts as a serializable snapshot of a domain object, storing only the necessary primitive data. The domain objects themselves are not directly serialized.
- **Builder (during restore)** – The `Restore()` methods gradually rebuild complex objects (e.g., `GameSession` → loot storage → dogs) and resolve cross‑references (loot IDs → pointers, session IDs → session pointers) using temporary lookup maps.
- **Factory** – The `Restore()` methods act as factories that produce domain objects from the representation data.
- **RAII** – Not directly present, but the representation classes manage no resources themselves – they rely on Boost.Serialization’s archive handling.

## Libraries Used

- **Boost.Serialization** – Core serialization engine. Used via `boost::serialization::access` and `serialize()` member functions. Supports versioning and archive types (binary, text, XML).
- **C++17 / C++20 STL** – `std::vector`, `std::unordered_map`, `std::string`, `std::string_view`, `std::find_if`, `std::distance`, ranges (`std::views::values`).
- **Project‑internal** – `model::Game`, `model::GameSession`, `model::Dog`, `loot::LootStorage`, `app::Players`, `extra_data::GameExtraData`, `boost_logger` for error reporting.

## Files Summary

| File | Purpose |
|------|---------|
| `game_repr.h` | Top‑level `GameRepr` that aggregates session and player representations and orchestrates full state restoration. |
| `game_session_repr.h` | Defines `GameSessionRepr` and `DogRepr`. Handles session reconstruction, loot pointer mapping, and dog restoration. |
| `loot_repr.h` | Defines `LootStorageRepr` and `LootObjectRepr`. Serializes loot storage state and rebuilds loot objects with correct type pointers. |
| `players_repr.h` | Defines `PlayersRepr` and `PlayerRepr`. Serializes player collection, including player‑to‑session binding and authentication tokens. |

## Extra Data

### Dependencies on Other Modules

- **Game model** (`model::Game`, `model::GameSession`, `model::Dog`, `model::Map`) – provides the domain classes that are being serialized.
- **Loot system** (`loot::LootStorage`, `loot::LootObject`, `loot::LootObjectId`) – loot state representation.
- **Player management** (`app::Players`, `app::Player`) – player collection.
- **Extra data** (`extra_data::GameExtraData`) – provides per‑map loot type vectors used to map type indices back to pointers.
- **Logging** (`boost_logger`) – used for error reporting during restore.

### Integration Example

```cpp
#include "serialize_game_save/game_repr.h"
#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <fstream>

// Saving
void SaveGame(const model::Game& game, const app::Players& players, const std::string& path) {
    serialize_game_save::GameRepr repr(game, players);
    std::ofstream ofs(path);
    boost::archive::text_oarchive oa(ofs);
    oa << repr;
}

// Loading
void LoadGame(model::Game& game, app::Players& players, boost::asio::io_context& ioc,
              const std::string& path) {
    std::ifstream ifs(path);
    boost::archive::text_iarchive ia(ifs);
    serialize_game_save::GameRepr repr;
    ia >> repr;
    repr.Restore(game, players, ioc);
}
```

### Important Restoration Order

1. `GameRepr::Restore()` loops over `GameSessionRepr` entries.
2. For each session, `GameSessionRepr::Restore()`:
   - Finds the corresponding `model::Map` by ID.
   - Creates a new `model::GameSession` (with a fresh ticker, loot generator, etc.).
   - Restores loot storage first, obtaining an `id_to_loot` map.
   - Restores dogs, passing the loot map so that bag pointers are resolved.
   - Adds the session to `model::Game` and stores the raw pointer in `sessions_by_id`.
3. After all sessions are restored, `PlayersRepr::Restore()` uses `sessions_by_id` to attach each player to the correct session.

### Error Handling

- If a map ID referenced by a saved session does not exist in the current game, a `std::runtime_error` is thrown after logging the error.
- If a loot type index is out of range (should not happen with consistent configuration), the loot object’s `loot_data_ptr` remains `nullptr`.

---

*This module is part of a larger Game Server project – a multiplayer online game where players control dogs, collect loot, and compete on procedurally generated maps. Persistent state saving allows games to be stopped and resumed across server restarts.*
