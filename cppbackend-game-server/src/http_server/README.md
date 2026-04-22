# HTTP Server and API Module

The `/src/http_server/` directory contains the complete HTTP server infrastructure for the Game Server project: asynchronous request handling, API routing with path parameters, JSON serialization, static file serving, request logging, and integration with the game application layer.

## Code Description

- **HTTP Server Core** (`http_server.cpp/h`) – Low‑level asynchronous HTTP server built on Boost.Beast. Provides `Listener` (accepts connections) and `Session` (handles one client connection). Uses `boost::asio::strand` for thread‑safe per‑connection processing. Implements read/write timeouts and graceful shutdown.

- **Request Dispatcher** (`request_handler.cpp/h`) – Main entry point for all HTTP requests. Determines whether a request targets the API (`/api/v1/...`) or static files. For API requests, dispatches through a `boost::asio::strand` to serialise access to the game state. For static files, serves content from the configured `www_root` with path traversal protection and MIME type detection. Manages the game ticker for automatic state updates.

- **Logging Decorator** (`logging_request_handler.h`) – Wraps any request handler to log incoming requests and outgoing responses. Logs client IP, method, target, response status code, response time (ms), and content type. Uses the project’s `boost_logger`.

- **API Router** (`api_router.cpp/h`) – Tree‑based HTTP router supporting static routes and dynamic path parameters (e.g., `/api/v1/maps/:id`). Provides method validation, authentication requirement checks, Content‑Type header validation and automatic extraction of URL parameters into `RequestContext`. Handles the special `/api/v1/game/tick` endpoint blocking when auto‑tick is enabled.

- **API Handlers** (`api_handler.cpp/h`) – Implements all game API endpoints:
  - `GET /api/v1/maps` – list all maps (brief)
  - `GET /api/v1/maps/:id` – full map details including roads, buildings, offices, loot types
  - `POST /api/v1/game/join` – join a game (creates player, returns token)
  - `GET /api/v1/game/players` – list players in the current session
  - `GET /api/v1/game/state` – full game state (players, positions, loot)
  - `POST /api/v1/game/action` – set player movement direction
  - `POST /api/v1/game/tick` – manual game tick (only when auto‑tick is disabled)
  - `GET /api/v1/game/records` – leaderboard with pagination (offset/limit)

  Includes a reusable `ParseJsonRequest` helper with optional validator.

- **HTTP Response Builder** (`http_response.cpp/h`) – Fluent builder for constructing `StringResponse` objects. Supports JSON responses, error responses with code/message, custom content types, cache control and `Allow` headers. Provides convenience functions for common error codes (BadRequest, MethodNotAllowed, InternalServerError).

- **API Serialization** (`serialize_api.cpp/h`) – Converts game domain objects to Boost.JSON values. Serialises maps, roads, buildings, offices, loot types, player state (position, speed, direction, bag, score) and lost loot objects. Uses `tag_invoke` overload for `app_geom::Position2D` to round coordinates to two decimal places.

## Patterns Used

- **Decorator** – `LoggingRequestHandler` wraps any request handler, adding logging without modifying the original handler.
- **Builder** – `response::Builder` provides a fluent interface for constructing HTTP responses with various attributes.
- **Strategy** – The router’s `EndpointConfig` allows different handlers and validation strategies per route.
- **Chain of Responsibility** – `RequestHandler` decides between API routing and static file serving; API routing further delegates to the trie‑based `ApiRouter`.
- **Template Method** – `SessionBase` defines the async read/write skeleton; derived `Session<RequestHandler>` implements the pure virtual `HandleRequest`.
- **Factory** – `http_server::ServeHttp` creates and runs a `Listener` with the given handler.
- **RAII** – `beast::tcp_stream` manages socket lifetime and timeouts; `std::shared_ptr` ensures safe asynchronous callback lifetimes.
- **Tagged Type / ADL** – `tag_invoke` overload for `Position2D` customises JSON serialisation without modifying the original class.

## Libraries Used

- **Boost.Beast** – HTTP protocol, async read/write, `tcp_stream`, `flat_buffer`, HTTP fields.
- **Boost.Asio** – `io_context`, `strand`, `ip::tcp`, timers, `dispatch` / `post`.
- **Boost.JSON** – Parsing and serialisation of JSON for API requests and responses.
- **C++17 / C++20 STL** – `std::filesystem`, `std::unordered_map`, `std::optional`, `std::function`, `std::chrono`, `std::ranges`.
- **Project‑internal** – `boost_logger` (logging), `constants.h` (API paths, error codes, content types), `utils.h` (URL decoding, MIME types, path traversal check, direction conversion), `ticker.h` (periodic game update), `application.h` (game logic), `model::Game`, `app::Players`, `serialize_game_save` (indirectly).

## Files Summary

| File | Purpose |
|------|---------|
| `api_handler.cpp/h` | Implements all game API endpoints (maps, join, state, action, tick, records). Contains JSON parsing helpers and delegates to `serialize_api`. |
| `api_router.cpp/h` | Trie‑based router with path parameters, method validation, auth, content‑type checks. Manages `RequestContext`. |
| `http_response.cpp/h` | Fluent builder for HTTP responses. Supports JSON, errors, custom headers, and convenience functions. |
| `http_server.cpp/h` | Low‑level async HTTP server: `Listener` (accepts connections), `Session` (per‑connection read/write loop), `ServeHttp` entry point. |
| `logging_request_handler.h` | Decorator that logs request details (IP, method, target) and response (status, time, content type). |
| `request_handler.cpp/h` | Main dispatcher: routes API requests via strand, serves static files with security checks, manages game ticker. |
| `serialize_api.cpp/h` | Converts game model objects (maps, loot, dogs, game state) to Boost.JSON. Includes custom `tag_invoke` for `Position2D`. |

## Extra Data

### API Endpoints Summary

| Method | Path                     | Auth | Description                     |
|--------|--------------------------|------|---------------------------------|
| GET    | `/api/v1/maps`           | No   | List all maps (id, name)        |
| GET    | `/api/v1/maps/:id`       | No   | Full map details                |
| POST   | `/api/v1/game/join`      | No   | Join game, returns token        |
| GET    | `/api/v1/game/players`   | Yes  | List players in current session |
| GET    | `/api/v1/game/state`     | Yes  | Full game state (positions, loot)|
| POST   | `/api/v1/game/action`    | Yes  | Set movement direction          |
| POST   | `/api/v1/game/tick`      | No   | Manual game tick (if auto disabled) |
| GET    | `/api/v1/game/records`   | No   | Leaderboard (offset, maxItems)  |

### Authentication

The API uses Bearer tokens. Clients receive a token upon joining a game (`POST /api/v1/game/join`). For endpoints that require authentication (`requires_auth = true`), the token must be provided in the `Authorization` header:
```
Authorization: Bearer <token>
```

### Static File Serving

- Root directory is set via `--www-root` command‑line argument.
- Serves files with correct MIME types (based on extension).
- Supports `index.html` for directory requests.
- Path traversal attacks are prevented by verifying that the canonicalised path remains inside `www_root`.

### Integration with Main Server

```cpp
#include "http_server.h"
#include "request_handler.h"
#include "logging_request_handler.h"
#include "game_app/application.h"

// Create application and io_context
app::Application app(argc, argv);
net::io_context ioc;

// Create request handler (with logging decorator)
using Handler = server_logging::LoggingRequestHandler<http_handler::RequestHandler>;
Handler handler{http_handler::RequestHandler{app, ioc}};

// Start HTTP server on endpoint
http_server::ServeHttp(ioc, {net::ip::make_address("0.0.0.0"), 8080}, handler);

// Run io_context
ioc.run();
```

### Logging Example

Request and response logs produced by `LoggingRequestHandler` (using the project’s JSON logger):

```json
{
  "timestamp": "2025-01-15T12:34:56.789",
  "data": {
    "ip": "192.168.1.100",
    "target": "/api/v1/game/state",
    "method": "GET"
  },
  "message": "Request received"
}
{
  "timestamp": "2025-01-15T12:34:56.795",
  "data": {
    "response_time_ms": 6,
    "status": 200,
    "content_type": "application/json"
  },
  "message": "Response sent"
}
```

### Router Example

Adding a custom route to `ApiHandler`:

```cpp
router_->AddRoute("/api/v1/admin/reset", {
    .allowed_methods = {http::verb::post},
    .requires_auth = true,
    .handler = [this](const RequestContext& ctx, std::string_view) {
        // reset logic
        return response::Builder::MakeJson(ctx.req, json::object{});
    }
});
```

### Environment / Configuration

- The server’s listening address and port are configured outside this module (in `main.cpp`).
- Static files root, tick period, and other options come from parsed command line.
- API base path is defined in `constants.h` as `api_paths::API_PREFIX = "/api/v1"`.

---

*This module is part of a larger Game Server project – a multiplayer online game where players control dogs, collect loot, and compete on procedurally generated maps. The HTTP server provides the Web API for clients to interact with the game.*
