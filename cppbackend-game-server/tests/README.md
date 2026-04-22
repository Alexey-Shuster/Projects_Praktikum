# Game Server Tests

This folder contains the unit test suite for the **Game Server** project.  
All tests are written using the **Catch2** framework with `BDD` approach and cover core components such as collision detection, loot generation, game model logic, database mocking and state serialization.

## Test Files Overview

| File | Description |
|------|-------------|
| `collision-detector-tests.cpp` | Tests for geometry‑based collision detection between gatherers (dogs) and items. Verifies edge cases (zero movement, diagonal paths, exact boundaries). |
| `loot-generator-tests.cpp` | Tests for the loot generation algorithm, including time‑based spawn rates, probability handling, and custom random generators. |
| `game-model-tests.cpp` | Tests for game map management, game session creation, session limits (max players), and updating all sessions. Uses Boost.Asio `io_context`. |
| `database_tests_local.cpp` | Tests for the in‑memory `TestPlayerScoreRepository` (pagination, sorting, upsert) and `TestUnitOfWork` / `TestDatabase` mocks. |
| `test_database.h` | Header providing mock database implementations (`TestPlayerScoreRepository`, `TestUnitOfWork`, `TestDatabase`) for isolated testing without a real PostgreSQL connection. |
| `state-serialization-tests.cpp` | Tests for saving/restoring game state using Boost.Serialization. Covers `DogRepr`, `LootStorageRepr`, `GameSessionRepr`, `PlayersRepr` and full `GameRepr`. |

## Building & Running the Tests

The tests are only built when CMake configuration is set to `Debug` or `RelWithDebInfo` (see `CMakeLists.txt`). Each test is a separate executable:

```bash
# Build all tests
cmake --build . --target game_model_tests loot_generator_tests collision_detection_tests state-serialization-tests database_tests_local

# Run individual test executables
./bin/game_model_tests
./bin/loot_generator_tests
./bin/collision_detection_tests
./bin/state-serialization-tests
./bin/database_tests_local
```

## Dependencies

- **Catch2** – header‑only test framework (provided via Conan)
- **Boost** – serialization, asio (for game‑model tests)
- The test code links against the corresponding server libraries (`Common_Lib`, `Game_Model_Lib`, `Game_DB_Lib`, `Game_Repr_Lib`).

## Notes

- Floating‑point comparisons use a tolerance of `1e‑10` (defined in `common_values::DOUBLE_ABS_TOLERANCE`).
- Database tests are purely in‑memory – no external database required.
- State serialization tests verify that objects can be round‑tripped through Boost.TextArchive.

For any questions or test failures, please refer to the server’s main documentation or open an issue.
