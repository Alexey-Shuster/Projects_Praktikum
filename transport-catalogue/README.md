# Transport Directory System

A C++17 application that builds a public transport directory from JSON input, supports queries for stops, bus routes and routing between stops and renders an interactive SVG map of the route network.

## Features

- **JSON input/output** – reads `base_requests` (stops, routes, distances) and `stat_requests` from `std::cin`; outputs JSON answers.
- **Stop and route management** – stores stops with geographic coordinates, routes with lists of stops, real distances between stops, and computes route curvature.
- **Routing between stops** – builds a directed weighted graph (vertices = waiting/boarding states) and finds optimal paths using bus wait times and travel speeds.
- **Map rendering** – generates an SVG map with configurable colors, line widths, label fonts, and projections from geographic coordinates to screen space.
- **Error handling** – returns `"not found"` messages for invalid stop/bus names in queries.
- **Modular design** – separates JSON parsing, catalogue storage, graph routing, and SVG rendering.

## Dependencies

- **C++17** compiler (GCC, Clang, MSVC)
- **CMake 3.10+**
- No external libraries – all components (JSON, SVG, graph, router) are implemented from scratch.

## Building

```bash
git clone <repo>
cd <repo>
mkdir build && cd build
cmake ..
cmake --build . --config Release
```

The executable is named `transport_directory` (or `transport_directory.exe` on Windows).

## Usage

The program reads a JSON document from `stdin` and writes the response to `stdout`. The JSON must contain two top-level arrays:

- `"base_requests"` – definition of stops and routes.
- `"stat_requests"` – queries for stop info, bus info, route between stops, or map SVG.

Example input structure:

```json
{
  "base_requests": [
    { "type": "Stop", "name": "A", "latitude": 55.0, "longitude": 37.0, "road_distances": { "B": 500 } },
    { "type": "Stop", "name": "B", "latitude": 55.1, "longitude": 37.1, "road_distances": {} },
    { "type": "Bus", "name": "123", "is_roundtrip": false, "stops": ["A", "B"] }
  ],
  "stat_requests": [
    { "id": 1, "type": "Stop", "name": "A" },
    { "id": 2, "type": "Bus", "name": "123" },
    { "id": 3, "type": "Route", "from": "A", "to": "B" },
    { "id": 4, "type": "Map" }
  ],
  "render_settings": { ... },
  "routing_settings": { "bus_wait_time": 6, "bus_velocity": 40 }
}
```

Run:

```bash
./transport_directory < input.json > output.json
```

### Query types

| Type   | Description                                 | Output fields                         |
|--------|---------------------------------------------|---------------------------------------|
| `Stop` | Lists buses that stop at the given stop.    | `buses` array                         |
| `Bus`  | Returns route statistics.                   | `curvature`, `route_length`, `stop_count`, `unique_stop_count` |
| `Route`| Finds optimal path between two stops.       | `total_time`, `items` (Wait/Bus steps) |
| `Map`  | Returns an SVG map of all routes.           | `map` (string containing SVG)         |

## Project Structure

- `main.cpp` – entry point: loads JSON, builds catalogue, processes requests, prints output.
- `json.h` / `json.cpp` – DOM‑style JSON parser and serializer (supports `null`, numbers, strings, bools, arrays, objects).
- `json_builder.h` / `json_builder.cpp` – fluent builder for constructing JSON values.
- `json_reader.h` / `json_reader.cpp` – orchestrates reading of base requests and stat requests; calls the catalogue and router.
- `transport_catalogue.h` / `transport_catalogue.cpp` – stores stops, routes, and real distances; computes route lengths and curvature.
- `transport_router.h` / `transport_router.cpp` – builds a graph (wait vertex + stop vertex per stop) and uses a generic `Router` to find shortest paths.
- `router.h` – generic Floyd‑Warshall‑like router for directed weighted graphs (non‑negative weights).
- `graph.h` – `DirectedWeightedGraph` template (adjacency list).
- `map_renderer.h` / `map_renderer.cpp` – converts geographic coordinates to SVG points using `SphereProjector`; draws polylines for routes, circles for stops, and text labels.
- `svg.h` / `svg.cpp` – low‑level SVG builder with support for `circle`, `polyline`, `text`, and styling.
- `geo.h` / `geo.cpp` – `Coordinates` struct and haversine distance computation.
- `ranges.h` – helper for range‑based iteration over graph incidence lists.

## Configuration

### Render settings (JSON object)

| Field                 | Type               | Description                                 |
|-----------------------|--------------------|---------------------------------------------|
| `width`               | double             | SVG canvas width                            |
| `height`              | double             | SVG canvas height                           |
| `padding`             | double             | Padding around the map                      |
| `line_width`          | double             | Width of route polylines                    |
| `stop_radius`         | double             | Radius of stop circles                      |
| `bus_label_font_size` | int                | Font size for bus route labels              |
| `bus_label_offset`    | [dx, dy]           | Offset of bus label from stop point         |
| `stop_label_font_size`| int                | Font size for stop names                    |
| `stop_label_offset`   | [dx, dy]           | Offset of stop label                        |
| `underlayer_color`    | string or [r,g,b] or [r,g,b,a] | Background stroke color for labels |
| `underlayer_width`    | double             | Width of label underlayer                   |
| `color_palette`       | array of colors    | Colors assigned to routes cyclically        |

### Routing settings

| Field            | Type   | Description                                 |
|------------------|--------|---------------------------------------------|
| `bus_wait_time`  | int    | Waiting time (minutes) at a stop            |
| `bus_velocity`   | double | Bus speed (km/h)                            |

## Error Handling

- If a stop or bus name in a `stat_request` does not exist, the response contains `"error_message": "not found"`.
- Invalid positions or missing required fields cause parsing errors (thrown as `ParsingError`).
- Graph routing returns `nullopt` when no path exists (outputs `"error_message": "not found"`).

## Example output

For the `Route` query above:

```json
{
  "request_id": 3,
  "total_time": 9.5,
  "items": [
    { "type": "Wait", "stop_name": "A", "time": 6 },
    { "type": "Bus", "bus": "123", "span_count": 1, "time": 3.5 }
  ]
}
```

## Limitations

- Bus routes are linear or circular; no branching or complex route patterns.
- All distances are integers (meters), travel times computed as `distance / velocity * 60`.
- The graph router uses O(V³) initialization (Floyd‑Warshall), suitable for up to a few thousand vertices.
- SVG output is not interactive; it is a static image.

## License

This project is provided for educational purposes. Use and modify freely.
