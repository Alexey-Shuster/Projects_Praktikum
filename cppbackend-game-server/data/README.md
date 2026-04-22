# Game Server Configuration Files

This directory contains JSON configuration files for the **Game Server**.  
They define game world parameters, map layouts, loot types, roads, buildings and offices.

## Files

### `config.json`
The **default production configuration**.  
Used when running the server normally. It includes:

- `defaultDogSpeed` – base movement speed for dogs.
- `lootGeneratorConfig` – how often loot spawns (`period`) and spawn probability.
- `dogRetirementTime` – time (seconds) after which a dog is removed.
- `maps` – array of game maps, each containing:
  - `id`, `name`
  - `lootTypes` (name, 3D asset file, color, scale, value)
  - `roads` (horizontal/vertical road segments)
  - `buildings` (obstacles with x, y, width, height)
  - `offices` (spawn points for bots/players)

### `debug.json`
A **debug / testing configuration** with faster gameplay:

- Higher `defaultDogSpeed` (5.0)
- Very fast loot generation (`period: 0.1`, `probability: 1.0`) – loot appears almost instantly
- Shorter `dogRetirementTime` (10.0)
- A single simple `"Debug Map"` with a basic road grid and one building/office

## Usage

- To run the server with the **production** config:  
  `./game_server --config config.json`

- To run in **debug** mode:  
  `./game_server --config debug.json`

> **Note:** The server expects asset files (e.g., `assets/key.obj`, `assets/wallet.obj`) to be present in the paths referenced in the JSON.

## Modifying Maps

You can add new maps to `config.json` by following the existing structure.  
Each map must have unique `id`. Roads are defined by either:
- `x0, y0, x1` (horizontal road from `(x0,y0)` to `(x1,y0)`), or  
- `x0, y0, y1` (vertical road from `(x0,y0)` to `(x0,y1)`).

Buildings are axis‑aligned rectangles. Offices define a spawn point with an optional offset.
