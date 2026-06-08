# gbs-MetatilePlugin

**Version 4.3.0 — Requires GB Studio ≥ 4.3.0**

A GB Studio engine plugin that introduces metatile support, enabling scenes to use large, dynamically editable tilemaps backed by SRAM. The entire scene tilemap and its collision data are stored in SRAM at runtime, which allows tile changes to persist, changes to take effect while tiles are off-screen, and the possibility to dynamically generate or modify a scene's layout at any point.

The plugin ships with example projects for the 8px and 16px metatile modes, as well as a 16px variant combined with screen-scroll functionality.

---

## Table of Contents

1. [Concepts](#concepts)
2. [Project Setup](#project-setup)
3. [Engine Settings](#engine-settings)
4. [Size Limits and Restrictions](#size-limits-and-restrictions)
5. [Events Reference](#events-reference)
6. [Engine Fields Reference](#engine-fields-reference)
7. [Inner Workings](#inner-workings)

---

## Concepts

### What is a Metatile?

A metatile is a logical grouping of one or more raw GB tiles treated as a single addressable unit. Instead of storing raw tile indices for every cell of a large map, the plugin stores one metatile ID per cell. Each ID points to a position inside a dedicated **metatile scene** that defines what raw tile(s) that ID actually looks like. This drastically reduces the amount of data the scene tilemap needs to hold and makes it easy to replace large areas of a scene by changing only a handful of metatile IDs.

### Two Metatile Sizes

The plugin supports two metatile sizes, selected globally via the **Size in pixels of one metatile** engine setting:

| Mode | Metatile footprint | Map data limit | Collision data limit |
|------|--------------------|----------------|----------------------|
| **8px** | 1×1 raw tile (8×8 px) | 7 936 bytes | 256 bytes |
| **16px** | 2×2 raw tiles (16×16 px) | 7 168 bytes | 1 024 bytes |

In **8px** mode each metatile maps directly to a single tile, giving you per-tile granularity. In **16px** mode each metatile covers a 2×2 block, halving the resolution of the map data but allowing richer individual metatile designs (each metatile can use up to four distinct raw tiles).

### SRAM Layout

The plugin reserves the first SRAM bank (bank 0) exclusively for metatile data:

```
0xA000  ┌─────────────────────────────┐
        │  Map data (metatile IDs)    │  8px: up to 7 936 bytes
        │                             │  16px: up to 7 168 bytes
        ├─────────────────────────────┤
        │  Collision data             │  8px: 256 bytes (one per metatile ID)
        │                             │  16px: 1 024 bytes (four sub-tiles × 256)
0xBFFF  └─────────────────────────────┘
```

Save/load data occupies SRAM banks 1–3, leaving bank 0 untouched. This means metatile map modifications are **not** wiped by a save or load operation — they persist across the entire session until the scene is re-initialised.

---

## Project Setup

### 1. Create a Common Tileset

Design a single tileset image that contains every unique raw tile your project needs. Both the metatile scene and all scenes that use metatiles must share the same tileset so that tile indices are consistent.

### 2. Create a Metatile Scene

The metatile scene is a special GB Studio scene whose only purpose is to define the available metatiles. It is **never loaded or visited at runtime** — the compiler reads it only to build the metatile lookup table.

- **8px mode**: The metatile scene must be exactly **128 pixels wide** (16 tiles). Each individual tile in the scene is one metatile. Up to 256 metatiles can be defined (tiles 0–255).
- **16px mode**: The metatile scene must be exactly **256 pixels wide** (32 raw tiles / 16 metatile columns). Each 2×2 block of tiles is one metatile. Up to 256 metatiles can be defined.

Paint the metatile scene using the shared tileset. Set collision data on it as needed — this collision data is what gets loaded into SRAM at runtime.

### 3. Add Load Meta Tiles to the Main Scene

In the **On Init** script of each scene that should use metatiles, add a **Load meta tiles** event and point it at your metatile scene.

When the project is compiled, the compiler rewrites the scene's tilemap: every tile group (1×1 for 8px, 2×2 for 16px) is looked up in the metatile scene and replaced by its corresponding metatile ID. The result is a compact array of IDs that is stored in ROM and copied into SRAM at runtime.

**Optional matching flags** on the Load meta tiles event:
- **Must match metatile color attributes** — differentiates metatiles that share the same tile pattern but use different CGB palette attributes.
- **Must match metatile collision** — differentiates metatiles that share the same tile/color pattern but have different collision data.

> If a tile in your main scene is not found in the metatile scene, the compiler will throw an error identifying the problematic coordinate.

### 4. Enable Metatile Events (optional)

If you want scripts to react when the player enters or collides with specific metatiles, go to **Settings → Metatiles** and enable the relevant metatile event options for each scene type you use (see [Engine Settings](#engine-settings)).

### 5. Attach Scripts to Metatile Events (optional)

Use the **Attach a Script to a Metatile Event** event in the scene's init script to register a script that will run whenever the chosen metatile event fires.

---

## Engine Settings

These settings are found under **Settings → Metatiles** (and per scene-type groups).

### Global Settings

| Setting | Default | Description |
|---------|---------|-------------|
| **Size in pixels of one metatile** | 8px | Choose between `8px` (single-tile metatiles) and `16px` (2×2 tile metatiles). This must match how your metatile scene is laid out. Changing this value affects every scene in the project. |
| **Minimum metatile index to start checking for the entered metatile event** | 1 | Metatile IDs below this value are ignored by the Metatile Enter event. Setting this above 0 is a common optimisation to skip the "empty" metatile (ID 0). |
| **Minimum metatile index — down collision event** | 1 | Same threshold, applied to the Down collision event. |
| **Minimum metatile index — right collision event** | 1 | Same threshold, applied to the Right collision event. |
| **Minimum metatile index — up collision event** | 1 | Same threshold, applied to the Up collision event. |
| **Minimum metatile index — left collision event** | 1 | Same threshold, applied to the Left collision event. |

### Per Scene-Type Flags

Each scene type (Platformer, Top-Down, Adventure, Point & Click, SHMUP) has its own set of enable flags. None are enabled by default — only enable what your game actually uses to avoid unnecessary per-frame processing.

| Flag | Description |
|------|-------------|
| **Enable enter metatile event** | Fires when the player's bounding box transitions onto a new metatile. |
| **Enable collision metatile event** | Master switch for the directional collision events below. |
| ↳ **Enable down collision metatile event** | Fires when the player is blocked moving downward. |
| ↳ **Enable right collision metatile event** | Fires when the player is blocked moving rightward. |
| ↳ **Enable up collision metatile event** | Fires when the player is blocked moving upward. |
| ↳ **Enable left collision metatile event** | Fires when the player is blocked moving leftward. |

---

## Size Limits and Restrictions

### Scene Dimensions

The plugin stores one metatile ID per map cell in a flat SRAM array indexed as:

```
index = (y_in_metatiles × next_power_of_two(width_in_metatiles)) + x_in_metatiles
```

The scene dimensions must satisfy:

**8px mode:**
> `next_power_of_2(scene_width_in_tiles) × scene_height_in_tiles ≤ 7 936`
>
> Example: a 255-tile wide scene → next power of 2 is 256 → max height is 31 tiles (256 × 31 = 7 936).

**16px mode:**
> `next_power_of_2(scene_width_in_metatiles) × scene_height_in_metatiles ≤ 7 168`
>
> Example: a 127-metatile wide scene (254 raw tiles) → next power of 2 is 128 → max height is 56 metatiles (128 × 56 = 7 168). In raw tiles that is 254×112.

If a scene exceeds the limit the compiler throws an error with the exact numbers.

### Metatile Scene Width

| Mode | Required metatile scene width |
|------|-------------------------------|
| 8px  | 128 px (16 tiles) |
| 16px | 256 px (32 tiles / 16 metatile columns) |

### Commit Render Flag

Several events include a **Commit render** checkbox. When unchecked the metatile ID in SRAM is updated but the screen is not refreshed. Always leave this unchecked when the affected metatile position is off-screen — attempting to write off-screen tile data to VRAM causes visual corruption. Use the checked form only for positions that are currently visible.

### SRAM Bank 0 is Reserved

Do not use SRAM bank 0 for any other purpose. The plugin writes the entire metatile map and collision table there on scene init and at any point tile data is modified at runtime.

### Save/Load Compatibility

The plugin's save system uses SRAM banks 1–3. Metatile map data stored in SRAM bank 0 is **not** part of the saved state.

---

## Events Reference

All events appear under the **Meta Tiles** group in the script editor.

---

### Load meta tiles

**`EVENT_LOAD_META_TILES`**

The core setup event. Must be called in a scene's init script before any other metatile event runs.

At **compile time** it rewrites the scene's tilemap from raw tile indices to an array of metatile IDs. At **runtime** it copies the metatile collision table and the compressed tilemap from ROM into SRAM bank 0, then resets the scroll renderer so the new metatile data is used immediately.

| Field | Description |
|-------|-------------|
| Metatile Scene | The scene that defines the metatile set. |
| Must match metatile color attributes | Include CGB palette attributes in the metatile lookup key. |
| Must match metatile collision | Include collision data in the metatile lookup key. |

<img width="739" height="130" alt="image" src="https://github.com/user-attachments/assets/aad82d0e-f5a5-44b6-a0ea-b2d042aa8ea1" />

---

### Get meta tile at position

**`EVENT_GET_META_TILE_AT_POS`**

Reads the metatile ID currently stored in SRAM at the given tile position and writes it into a variable. Coordinates are in **tile units** (not pixels).

| Field | Description |
|-------|-------------|
| X | Horizontal tile position (8px: tile column; 16px: tile column, not metatile column). |
| Y | Vertical tile position. |
| Variable | Destination variable to receive the metatile ID (0–255). |

<img width="743" height="91" alt="image" src="https://github.com/user-attachments/assets/0db265a2-d7a4-4a9b-8e82-ab56b220aa27" />

---

### Get meta tile collision at position

**`EVENT_GET_META_TILE_COLLISION_AT_POS`**

Reads the collision byte for the tile at the given position from SRAM collision data and writes it into a variable. For 16px metatiles, the sub-tile within the 2×2 block is determined by the parity of X and Y.

| Field | Description |
|-------|-------------|
| X | Horizontal tile position. |
| Y | Vertical tile position. |
| Variable | Destination variable to receive the collision byte. |

<img width="742" height="95" alt="image" src="https://github.com/user-attachments/assets/d4ed93ae-e6e9-4b93-a323-17c940d9db50" />

---

### Replace meta tile

**`EVENT_REPLACE_META_TILE`** — auto-label: *Assign meta tiles*

Overwrites the metatile ID in SRAM at the given position and optionally re-renders the affected screen area.

| Field | Description |
|-------|-------------|
| X | Horizontal tile position. |
| Y | Vertical tile position. |
| Metatile Id | The new metatile ID to store (0–255). |
| Commit render | Check to immediately update the on-screen tile graphics. Leave unchecked for off-screen positions. |

<img width="731" height="135" alt="image" src="https://github.com/user-attachments/assets/c5c1e68a-d0f8-4e37-b58c-b431a1c1c6d1" />

---

### Reset meta tile

**`EVENT_RESET_META_TILE`** — auto-label: *Reset meta tiles*

Restores the metatile at the given position to the value that was originally loaded from ROM (i.e. the value set at scene init by Load meta tiles).

| Field | Description |
|-------|-------------|
| X | Horizontal tile position. |
| Y | Vertical tile position. |
| Commit render | Check to immediately update the on-screen tile graphics. Leave unchecked for off-screen positions. |

<img width="741" height="97" alt="image" src="https://github.com/user-attachments/assets/a33d93e8-d13a-4df1-8b80-2ba926d8d0ff" />

---

### Replace collision (16px metatile)

**`EVENT_REPLACE_COLLISION_16`**

Overwrites the collision bytes for all four sub-tiles of a 16px metatile ID in SRAM. Does not modify the visual tile data.

| Field | Description |
|-------|-------------|
| Metatile Id | The metatile ID whose collision data to change (0–255). |
| Collision Top Left | New collision byte for the top-left sub-tile. |
| Collision Top Right | New collision byte for the top-right sub-tile. |
| Collision Bottom Left | New collision byte for the bottom-left sub-tile. |
| Collision Bottom Right | New collision byte for the bottom-right sub-tile. |

<img width="740" height="96" alt="image" src="https://github.com/user-attachments/assets/8976b4f5-7a8f-42e4-9668-efc1d56533b9" />

---

### Replace collision (8px metatile)

**`EVENT_REPLACE_COLLISION_8`**

Overwrites the collision byte for a single 8px metatile ID in SRAM.

| Field | Description |
|-------|-------------|
| Metatile Id | The metatile ID whose collision data to change (0–255). |
| Collision | New collision byte. |

---

### Submap metatiles

**`EVENT_SUBMAP_METATILES`** — auto-label: *Submap metatiles*

Copies a rectangular region of metatile IDs from another scene's ROM tilemap directly into the active scene's SRAM map data, and optionally re-renders the affected rows on screen. This is useful for room transitions, destructible terrain sections, or procedural map assembly.

| Field | Description |
|-------|-------------|
| Scene | The source scene to copy metatile IDs from. |
| Source X / Source Y | Top-left tile coordinate in the source scene. |
| Destination X / Destination Y | Top-left tile coordinate in the active scene to paste into. |
| Width / Height | Size of the region to copy, in tile units (max 31 per axis). |
| Commit render | Check to re-render each copied row immediately. Leave unchecked for off-screen regions. |

<img width="742" height="149" alt="image" src="https://github.com/user-attachments/assets/6fb9d619-378c-4c27-a402-ebd32647708d" />

---

### Attach a Script to a Metatile Event

**`PM_EVENT_METATILE_SCRIPT`**

Registers a sub-script to run whenever the selected metatile event fires. Must be placed in the scene's init script (runs before the init fade). Only one script can be registered per event slot at a time; calling this again on the same event replaces the previous script.

| Field | Description |
|-------|-------------|
| Select Metatile Event | Choose which of the six event types to hook. |
| On Metatile Event (script) | The script to execute when the event fires. |

**Available event types:**

| Type | When it fires |
|------|---------------|
| Metatile Enter | The player's bounding box moves onto a tile whose metatile ID is ≥ `MIN_OVERLAP_METATILE`. Fires once per new tile entered. |
| Metatile Down Collision | The player is blocked by a tile when attempting to move downward, and the colliding metatile ID is ≥ `MIN_DOWN_COLLISION_METATILE`. |
| Metatile Right Collision | Player is blocked moving rightward. |
| Metatile Up Collision | Player is blocked moving upward. |
| Metatile Left Collision | Player is blocked moving leftward. |
| Metatile Any Collision | Fires on any of the four directional collision events. |

When the Enter event fires, the engine fields **Entered Metatile Id**, **Entered Metatile X position**, and **Entered Metatile Y position** are populated. When a Collision event fires, the **Collided Metatile Id**, **Collided Metatile X/Y position**, **Collided Metatile Direction**, and **Collided Metatile Source** fields are populated.

<img width="738" height="589" alt="image" src="https://github.com/user-attachments/assets/31169949-4a2d-4846-ac11-b6c887d5b19f" />

<img width="745" height="213" alt="image" src="https://github.com/user-attachments/assets/2494bae8-3f9d-4efd-b397-f4faa3027ef6" />

---

### Remove a Script from Metatile Event

**`PM_EVENT_METATILE_SCRIPT_CLEAR`**

Unregisters the script currently attached to the selected metatile event slot, stopping it from firing on subsequent occurrences.

| Field | Description |
|-------|-------------|
| Select Metatile Event | The event slot to clear. |

---

## Engine Fields Reference

These read-only runtime fields are populated by the engine before the attached metatile event script runs. Access them via **Engine Field Value** in the script editor.

### Enter Event Fields

| Field | Description |
|-------|-------------|
| `overlap_metatile_id` | Metatile ID of the tile the player has just entered. |
| `overlap_metatile_x` | Tile X coordinate of the entered metatile. |
| `overlap_metatile_y` | Tile Y coordinate of the entered metatile. |

### Collision Event Fields

| Field | Description |
|-------|-------------|
| `collided_metatile_id` | Metatile ID of the tile the player collided with. |
| `collided_metatile_x` | Tile X coordinate of the collided tile. |
| `collided_metatile_y` | Tile Y coordinate of the collided tile. |
| `collided_metatile_dir` | Direction of the collision (matches GB Studio direction constants). |
| `collided_metatile_source` | Reserved for internal use; indicates the source of the collision check. |

---

## Inner Workings

### Compile-Time Tilemap Rewriting

The most important step happens entirely at compile time inside `eventLoadMetaTiles.js`. When the compiler encounters a **Load meta tiles** event for a scene it:

1. Reads the source scene's raw tilemap and (optionally) its CGB attribute map and collision array.
2. Reads the metatile scene and builds a lookup dictionary: for each metatile position in the metatile scene it computes a lookup key from the tile indices of the raw tiles at that position (and, if requested, their color attributes and/or collision bytes), then maps that key to the metatile's index (0-based, left-to-right, top-to-bottom).
3. Iterates over every metatile-sized cell in the source scene, computes the same lookup key, and finds the matching metatile index. If no match is found the compiler throws an error.
4. Replaces the scene's tilemap data with the array of metatile IDs and clears the collision array (collision is now handled entirely by the SRAM data at runtime).

The result is that the ROM tilemap for a metatile scene is an array of 1-byte metatile IDs instead of raw tile indices. This is much smaller for large scenes and is what gets placed in ROM.

### Runtime Initialisation (vm_load_meta_tiles)

When **Load meta tiles** runs at runtime it:

1. Reads the metatile scene struct from ROM to obtain pointers to its tilemap, CGB attribute map, and collision arrays.
2. Calls `load_meta_tiles()`, which:
   - Copies the metatile scene's collision data into `sram_collision_data` at `SRAM_COLLISION_DATA_PTR` using `MemcpyBanked`.
   - Computes `image_tile_width_bit` (the bit-shift equivalent of the next power of two of the scene width, used for fast row offset arithmetic).
   - Copies the scene's metatile ID tilemap row by row into `sram_map_data` at `SRAM_MAP_DATA_PTR`.
   - Calls `scroll_reset()` to force a full screen redraw using the new metatile data.

After this point `metatile_collision_bank` is nonzero, which acts as a flag throughout the engine that metatile mode is active.

### Rendering (scroll.c)

The plugged scroll system overrides `load_metatile_row` and `load_metatile_col` which are called by the standard GB Studio scroll update loop whenever a new row or column of tiles needs to be drawn. Instead of reading tile indices directly from a ROM tilemap these functions:

1. Look up the metatile ID for each cell from `sram_map_data`.
2. Derive the actual raw tile index from the metatile scene's ROM tilemap using the ID (and the sub-tile offset for 16px mode).
3. Write those tile indices into VRAM background map.

For CGB the same process is repeated for the attribute map with VRAM bank 1 selected.

This indirection means that any change to `sram_map_data` — whether from **Replace meta tile**, **Reset meta tile**, or **Submap metatiles** — is automatically picked up the next time that row or column scrolls into view. The optional **Commit render** flag triggers an immediate forced redraw of the affected row rather than waiting for natural scroll-driven rendering.

### Collision Detection (collision.c)

The plugged collision functions (`tile_col_test_range_y`, `tile_col_test_range_x`) check `metatile_collision_bank` at the start. When it is nonzero they read collision bytes from `sram_collision_data` instead of the ROM collision array:

- **8px mode**: index = `sram_map_data[METATILE_MAP_OFFSET(tx, ty)]`
- **16px mode**: index = `get_metatile_tile(metatile_offset, tile_offset)` — a two-level lookup that finds the metatile ID first, then computes the sub-tile's position within the 1024-byte collision table using the metatile ID's high and low nibbles.

This means that calling **Replace collision** changes how the player physically interacts with a tile type globally, not just at one position. To change collision at a single position you would first call **Replace meta tile** to assign a different metatile ID at that coordinate and then optionally adjust the collision for that ID.

### Metatile Enter Event

When the enter metatile event is enabled for a scene type, the state update loop calls `metatile_overlap_at_intersection` once per frame when the player is grid-aligned. That function:

1. Computes the metatile coordinate at the player's centre.
2. Reads the metatile ID from `sram_map_data`.
3. Compares the ID to `MIN_OVERLAP_METATILE`. If the ID is below the threshold the check is skipped.
4. Compares the current metatile coordinate to the previously recorded one. If the player has moved to a new metatile, it stores the ID, X, and Y into `overlap_metatile_id/x/y` and fires the registered script event via `script_event_t`.

### Metatile Collision Events

When a directional collision event is enabled, the state update loop calls `on_player_metatile_collision` immediately after detecting a blocking tile. That function:

1. Reads the metatile ID at the blocking tile's position.
2. Compares it to the minimum index threshold for that direction.
3. If the ID is at or above the threshold and the collision has not already been reported for the same tile this frame (debounced via a small cache), it stores the result into `collided_metatile_id/x/y/dir/source` and fires the registered script event.

A call to `reset_collision_cache` clears the debounce state when the player is no longer blocked in that direction, ensuring the event can fire again the next time the player tries to move into the same tile.

---

## Media

https://github.com/user-attachments/assets/854163c6-284a-4cfd-9dc4-24ba58504804

![image](https://github.com/user-attachments/assets/7fb07219-327f-4818-ba40-7e9a12484f4f)

https://github.com/user-attachments/assets/26751480-4c02-45ea-af46-df208dc619e9

![image](https://github.com/user-attachments/assets/61145b99-31a3-4ed2-912f-bbd7e786c066)

https://github.com/user-attachments/assets/72537786-be55-4dd1-968d-01b5c69c12fc

![image](https://github.com/user-attachments/assets/f6491b28-919a-4043-999f-effef4ac3023)

![SceneRendering3](https://github.com/user-attachments/assets/570cead9-04eb-4df7-8af4-e04235fbccb2)
