# TikTok Live Dock — OBS 32+ Plugin (fork)

This fork turns the original standalone Streamlabs/TikTok stream-key generator
into a **native OBS Studio (32+) plugin**: a single dock panel inside OBS that
auto-generates your TikTok LIVE stream key, sets the title, picks the
game/category, and starts/stops the actual OBS stream — no more copy-pasting
a key into Settings > Stream.

## What changed vs. the original standalone app

The original app (`Stream.py`, `TokenRetriever.py`,
`StreamLabsTikTokStreamKeyGenerator.py`, `Updater.py`) is a separate PySide6
GUI you had to run alongside OBS and then manually paste the generated
server/key into OBS. Those files are kept at the repo root for reference.

The new native plugin (`src/`, `data/`) lives *inside* OBS as a dock:

| Capability | Original app | This plugin |
|---|---|---|
| Generate stream key | Yes, via Streamlabs API | Yes, same Streamlabs API |
| Set title | Yes, in-app field | Yes, in-dock field |
| Pick game/category | Yes, autocomplete search | Yes, autocomplete combo box |
| Apply to OBS | Manual copy/paste | Automatic — writes the OBS streaming service directly |
| Start/stop stream | External to OBS | `Go Live` / `End Live` call OBS's own start/stop APIs |
| Distribution | Standalone executable | Installable via OBS 32's Plugin Manager |

## How it works

`src/StreamClient.{h,cpp}` is a direct C++ port of `Stream.py`, calling the
same licensed Streamlabs endpoints:

- `GET  /api/v5/slobs/tiktok/info?category=<query>` — category/game search
- `POST /api/v5/slobs/tiktok/stream/start` — creates the TikTok LIVE room, returns `rtmp` + `key`
- `POST /api/v5/slobs/tiktok/stream/{id}/end` — ends the room
- `GET  /api/v5/slobs/tiktok/info` — account/eligibility info

`src/TikTokDock.{h,cpp}` is the Qt dock UI registered via
`obs_frontend_add_dock_by_id`. On "Go Live" it calls `StreamClient::startStream`,
then feeds the returned server/key straight into
`obs_service_create("rtmp_custom", ...)` + `obs_frontend_set_streaming_service`
+ `obs_frontend_streaming_start()`. It also listens for
`OBS_FRONTEND_EVENT_STREAMING_STOPPED` so the TikTok room is closed even if
you stop streaming from OBS's own controls instead of the dock's "End Live"
button.

## Requirements

- Streamlabs TikTok LIVE access (request it from Streamlabs; no follower minimum required)
- A Streamlabs API token (paste it into the dock's token field once — it's saved to the plugin's local config, not committed to the repo)
- Qt6 (Widgets + Network)
- OBS Studio 32+ source or dev package exporting `libobs` / `obs-frontend-api` CMake config files

## Building

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build build --config RelWithDebInfo
```

Install into your OBS plugins directory (or point `CMAKE_INSTALL_PREFIX` at
your OBS install) and the "TikTok Live" dock will appear under
**View > Docks** the next time OBS starts.

> If you prefer building from the official
> [obs-plugintemplate](https://github.com/obsproject/obs-plugintemplate)
> scaffolding (recommended for packaging/CI/signing), copy `src/` and
> `data/` into that template and merge its build-system helpers with the
> `CMakeLists.txt` here.

## Known limitations / next steps

- Token entry is manual paste-in for now — the original app's automatic
  browser/Streamlabs-desktop token retrieval (`TokenRetriever.py`) hasn't
  been ported to C++ yet. A good follow-up is wiring this through OBS's
  bundled CEF browser panel (same one `obs-browser` uses) instead of adding
  a Selenium/Chrome dependency to a compiled plugin.
- No auto-update mechanism (`Updater.py`'s job) — OBS 32's built-in Plugin
  Manager handles update checks once this is published there.
