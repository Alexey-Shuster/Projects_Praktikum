# Database Persistence Layer for Game Server

The `/src/game_db/` directory contains the database abstraction and persistence layer for the Game Server project. It provides connection pooling, unit of work (transaction) management, repository interfaces, and both in‑memory (for testing) and PostgreSQL‑backed implementations.

## Code Description

- **Connection pool** (`connection_pool.h`) – Thread‑safe pool of `pqxx::connection` objects. Clients borrow connections via `GetConnection()`, which returns a RAII wrapper that automatically returns the connection when destroyed. Supports blocking wait with timeout.
- **Database interface** (`database_interface.h`) – Abstract factory for creating `UnitOfWork` objects. Decouples the rest of the system from concrete database implementations.
- **Migrations** (`db_migrations.h`) – Idempotent schema setup: creates `player_scores` table and required indexes. Uses a connection from the pool.
- **Local (in‑memory) database** (`local_database.h`) – Pure in‑memory implementation with snapshot isolation. Each `UnitOfWork` gets a copy of the current data; commit atomically replaces the global snapshot. Includes a `PlayerScoreRepositoryLocal` that sorts in memory.
- **Mock database** (`mock_database.h`) – Dummy implementations for unit testing. All operations are no‑ops and return empty results.
- **Player domain** (`player_db.h`) – Defines `PlayerId` (a tagged UUID), `PlayerScore` struct, and `PlayerScoreRepository` abstract interface.
- **Pooled database** (`pooled_database.h`) – Concrete `DatabaseInterface` that creates `PooledUnitOfWork` objects. Each unit of work borrows a connection from the pool, starts a `pqxx::work` transaction, and returns the connection on destruction.
- **Repository implementations** (`repository_impls.h`) – PostgreSQL‑backed `PlayerScoreRepositoryRemote` using `pqxx::work::exec_params`. Handles parameterised queries and result set mapping.
- **Tagged UUID** (`tagged_uuid.h`) – Strong typedef for UUIDs based on `util::Tagged` and Boost.UUID. Provides `New()`, `ToString()`, `FromString()` and defaults to nil UUID.
- **Unit of work** (`unit_of_work.h`) – Abstract `UnitOfWork` interface with `PlayerScores()` accessor and `Commit()` method. Also provides `UnitOfWorkRemote` (a standalone transaction wrapper, kept for legacy/compatibility).
- **Dummy source** (`dummy.cpp`) – Empty compilation unit to force static library generation when no other source files are present.

## Patterns Used

- **Connection Pool** – Reuses a fixed number of database connections to reduce overhead (`connection_pool.h`). The RAII wrapper (`ConnectionWrapper`) automatically returns the connection.
- **Unit of Work** – Groups multiple repository operations into a single transaction (`UnitOfWork` interface; `PooledUnitOfWork`, `LocalUnitOfWork` implementations).
- **Repository** – Abstracts data access for `PlayerScore` entities (`PlayerScoreRepository` interface; remote and local implementations).
- **Abstract Factory** – `DatabaseInterface` provides a factory method `CreateUnitOfWork()`.
- **Snapshot Isolation** – `LocalDatabase` copies the entire data set for each unit of work, commits atomically via copy‑on‑write (shared pointer to immutable map).
- **Tagged Type (Strong Typedef)** – `TaggedUUID<Tag>` prevents accidental mixing of different identifier types.
- **RAII** – `ConnectionWrapper` returns connection to pool on destruction; `PooledUnitOfWork` rolls back transaction if `Commit()` is not called; mutex locks are automatically released.
- **Strategy** – Swappable database backends (real PostgreSQL, in‑memory, mock) via the `DatabaseInterface` abstraction.

## Libraries Used

- **libpqxx** – PostgreSQL C++ client library. Used for connections, transactions, and parameterised queries.
- **Boost.UUID** – UUID generation, string parsing, and formatting (`tagged_uuid.h`).
- **Boost.Log** – Referenced in `db_migrations.h` for logging (migration success message commented out).
- **C++17 / C++20 STL** – `std::mutex`, `std::condition_variable`, `std::map`, `std::shared_ptr`, `std::unique_ptr`, `std::vector`, `<algorithm>`.
- **PostgreSQL** – Underlying database server (via libpqxx).

## Files Summary

| File | Purpose |
|------|---------|
| `connection_pool.h` | Thread‑safe pool of PostgreSQL connections with RAII wrapper and timeout‑aware blocking acquisition. |
| `database_interface.h` | Pure abstract factory for creating `UnitOfWork` instances. |
| `db_migrations.h` | Idempotent creation of `player_scores` table and index. |
| `dummy.cpp` | Empty source file to ensure static library is built (workaround for build systems). |
| `local_database.h` | In‑memory database with snapshot isolation, local repository (in‑memory sorting), and `LocalUnitOfWork`. |
| `mock_database.h` | Mock implementations for unit testing (no persistent state, empty results). |
| `player_db.h` | Domain types: `PlayerId` (tagged UUID), `PlayerScore` struct, and `PlayerScoreRepository` interface. |
| `pooled_database.h` | Production database implementation using connection pool and libpqxx transactions. |
| `repository_impls.h` | PostgreSQL‑backed `PlayerScoreRepositoryRemote` with `exec_params` and result set mapping. |
| `tagged_uuid.h` | Strong typedef for UUIDs using `util::Tagged` and Boost.UUID utilities. |
| `unit_of_work.h` | Abstract `UnitOfWork` interface and a standalone `UnitOfWorkRemote` class (legacy). |

## Extra Data

### Environment Variables
- `GAME_DB_URL` – PostgreSQL connection string (e.g., `postgresql://user:pass@localhost/game`). Used to initialise the connection pool (the actual pool creation code is not shown in these headers but is expected to read this variable).

### Database Schema
The migrations create a single table:
```sql
CREATE TABLE IF NOT EXISTS player_scores (
    id             UUID PRIMARY KEY,
    player_name    TEXT NOT NULL,
    score          INTEGER NOT NULL CHECK (score >= 0),
    play_time_sec  DOUBLE PRECISION NOT NULL CHECK (play_time_sec >= 0)
);
CREATE INDEX idx_player_scores_sort ON player_scores (score DESC, play_time_sec ASC, player_name ASC);
```

### Usage Example (Production)
```cpp
#include "db/pooled_database.h"
#include "db/connection_pool.h"
#include "db/db_migrations.h"

// Create pool (connection string from env)
auto conn_string = std::getenv("GAME_DB_URL");
auto pool = std::make_shared<db::ConnectionPool>(10, [&]() {
    return std::make_shared<pqxx::connection>(conn_string);
});
db::RunMigrations(pool);

db::PooledDatabase db(pool);
auto uow = db.CreateUnitOfWork();
auto& repo = uow->PlayerScores();
repo.Save({playerId, "Alice", 100, 12.5});
uow->Commit();
```

### Usage Example (In‑memory for tests)
```cpp
#include "db/local_database.h"

db::LocalDatabase db;
auto uow = db.CreateUnitOfWork();
auto& repo = uow->PlayerScores();
repo.Save({playerId, "Bob", 50, 8.0});
uow->Commit(); // atomically makes changes visible
```

### Concurrency Notes
- `ConnectionPool` uses a mutex and condition variable; `GetConnection()` may block up to 60 seconds.
- `LocalDatabase` uses a mutex only during snapshot copy and commit; the snapshot itself is immutable and shared via `shared_ptr<const map>`, allowing lock‑free reads in the unit of work.
- `PooledUnitOfWork` holds a transaction on a specific connection; it is **not** thread‑safe – each unit of work should be used from a single thread.

---

*This persistence layer is part of a larger Game Server project – a multiplayer online game where players control dogs, collect loot, and compete on procedurally generated maps. The database stores high scores and player statistics.*
