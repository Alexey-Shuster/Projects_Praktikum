# Game Server Web Interface

This directory contains the main web frontend for the **Game Server**.  
The interface allows players to log in, select a game map and interact with the server’s REST API.

## File: `index.html`

The landing page of the game server’s web interface. It provides:

- **Player login form** – enter a name and choose a map to start a game.
- **Live API test panel** – test all game endpoints directly from the browser:
  - `POST /api/v1/game/join` – join a game session.
  - `GET /api/v1/game/players` – list all players.
  - `GET /api/v1/game/state` – get current game state.
  - `POST /api/v1/game/player/action` – send movement commands (L/R/U/D or stop).
  - `POST /api/v1/game/tick` – advance game time (debug/admin).
- **Navigation links** – example pages, load simulator, 404 test, path traversal tests.
- **Direct API access** – browse maps or fetch a specific map by ID.

### Dependencies

The page loads external resources via CDN:
- jQuery 3.6.0
- Font Awesome 6.4.0

Local assets (CSS, JS) are expected in `custom-index-data/`:
- `styles.css` – page styling.
- `main.js` – frontend logic (API calls, form handling, dynamic updates).

### Usage

1. Place `index.html` and the `custom-index-data/` folder in the server’s web root.
2. Ensure the server is running (default port 8080).
3. Open `http://<server-address>:8080/` in a browser (where `<server-address>` is localhost or your server's IP).

The page assumes the game server exposes the REST API under `/api/v1/` and serves static files from the root.

### Notes

- The “Auth Token” field is required for authenticated endpoints (obtained after a successful `/join` call).
- The “Proceed to Game” button appears after an API response – it redirects to `/game.html`.
- All relative links (e.g., `/about.html`, `/game.html`) must be implemented by the server or added as static files.

For full server API documentation, refer to the main project documentation.
