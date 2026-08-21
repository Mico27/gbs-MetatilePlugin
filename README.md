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
7. [Animating Tiles](#animating-tiles)
8. [Media](#media)
9. [Memory Footprint](#memory-footprint)
10. [Bank 0 (HOME) Usage](#bank-0-home-usage)
11. [Changelog](#changelog)

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

### Redraw meta tiles

**`EVENT_REDRAW_META_TILES`** — auto-label: *Redraw meta tiles*

Re-renders a rectangular region of the screen from the metatile map currently held in SRAM, rewriting both the tile indices and — on Game Boy Color — the colour attributes for every cell in the region.

Nothing in SRAM is modified; this event only pushes what is already there back into VRAM. Use it after changing map data with **Commit render** left unchecked, or after anything has written to the background tilemap in VRAM directly (a raw GBVM script, another plugin, a transition effect) and left cells showing the wrong tile.

| Field | Description |
|-------|-------------|
| X / Y | Top-left corner of the region to redraw, in **raw tile** coordinates — not metatiles, in both 8px and 16px mode. |
| Width / Height | Size of the region, also in raw tiles. |

> **On-screen regions only.** Like the **Commit render** flag, this event maps the coordinate to a VRAM cell by wrapping it (`x & 31`, `y & 31`), so a region that is not currently visible overwrites whichever cells happen to occupy that wrapped position and corrupts the display. Redraw only what is on screen.

Redrawing costs one VRAM write per cell, so keep regions as small as the change that prompted them rather than refreshing the whole screen.

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

## Animating Tiles

Animated water, waterfalls, flowers and similar effects are set up differently in a
metatile scene than in a stock GB Studio scene. This section explains why the usual
event does not work, and the techniques that do.

### Why "Replace Tile At Position" does not work here

The stock **Replace Tile At Position** event — and its **Sequence** variant, which is
what GB Studio normally uses for animated tiles — compiles to the GBVM opcode
`VM_REPLACE_TILE_XY`. That opcode is not given a VRAM tile slot. It is given a scene
coordinate, reads the scene's compiled tilemap in ROM at that coordinate to find out
*which tile slot is displayed there*, and only then overwrites that slot's pixel data:

```c
UWORD ofs = (image_tile_width * y) + x;
UBYTE target_tile = ReadBankedUBYTE(image_ptr + ofs, image_bank);   // <- tile index
SetBankedBkgData(target_tile, 1, /* new pixels */ ...);
```

In a scene using this plugin, that lookup returns the wrong number. The compiler has
rewritten the scene tilemap so that every cell holds a **metatile ID**, not a raw tile
index. `VM_REPLACE_TILE_XY` reads the metatile ID, treats it as a VRAM slot number, and
replaces the bitmap of a completely unrelated tile.

This cannot be worked around by picking a different coordinate, because the raw tile
index displayed at a position is not stored in the scene tilemap at all — the plugin
resolves it at draw time from the metatile scene's tilemap (`metatile_ptr`), indexed by
metatile ID. For the same reason, the stock **Get Tile At Position** event
(`VM_GET_TILE_XY`) returns a metatile ID rather than a tile index; use
[Get meta tile at position](#get-meta-tile-at-position) instead.

### Technique 1 — Replace the VRAM tile bitmap directly

Animating a tile on the Game Boy really means replacing the *pixel data* held in a VRAM
tile slot. Every cell on screen that references that slot changes in the same frame —
which is exactly the behaviour wanted for water, waterfalls or flowers. The tilemap is
never touched, so no metatile ID changes, nothing has to be re-rendered, and no
[Redraw meta tiles](#redraw-meta-tiles) call is needed.

So rather than asking "which tile is at x,y?", address the VRAM slot directly.

#### Keeping tile indices stable

Set the same **Common Tileset** (scene inspector → *Common Tileset*) on the metatile
scene and on every scene that uses metatiles. GB Studio then emits that tileset's tiles
first and in order, so a tile's VRAM slot is the same in every scene and is simply its
index in the common tileset image, counted left-to-right then top-to-bottom from 0.

> **Slots above 127.** The engine loads the first 128 tiles of a scene's tileset to VRAM
> slots 0–127, then places the remaining tiles in a second block that is aligned to end
> at slot 192 when the remainder is 64 tiles or fewer, and at slot 128 otherwise
> (`load_bkg_tileset` in `data_manager.c`). Index equals slot only for the first 128
> tiles, so keep animated tiles inside the first 128 entries of the common tileset and
> the question never comes up. If in doubt, confirm the slot in an emulator VRAM viewer
> (BGB, Emulicious).

#### Option A — gbs-replaceTilesetTilesPlugin

[gbs-replaceTilesetTilesPlugin](https://github.com/Mico27/gbs-replaceTilesetTilesPlugin)
wraps `VM_REPLACE_TILE` in a normal event, which is the simplest route.

1. Put the animation frames in their own tileset asset — e.g. four 8×8 frames of water
   laid out in a row, giving source indices 0, 1, 2, 3.
2. Add a **Replace Tileset Tiles** event and set:
   - **Tileset** — the animation frame tileset;
   - **Target Tile Index** — the VRAM slot of the tile being animated (add 2048 /
     `0x0800` to write into VRAM bank 1 on Game Boy Color);
   - **Source Offset Tile Index** — the frame to show, which can be a variable;
   - **Length** — how many consecutive tiles to copy (1 per frame here, or 4 for a
     16px metatile's whole 2×2 block if the frames are stored as contiguous quads).
3. Drive it from a loop — a **Set Timer Script**, or an *On Init* script ending in a
   loop of *Wait 8 frames* → advance the frame variable → **Replace Tileset Tiles**.

The **Replace Tileset Tiles Ex** event of that same plugin takes the source tileset as a
runtime bank + pointer pair instead of a build-time asset, if the animation set itself
has to be chosen at runtime.

#### Option B — raw GBVM

The same thing without the plugin, using a **GBVM Script** event. `VM_REPLACE_TILE` takes
its target and source indices as *variable references*, not immediates, so push them
first:

```
VM_PUSH_CONST   12                  ; target VRAM slot  -> .ARG1
VM_PUSH_CONST   0                   ; source tile index -> .ARG0
VM_REPLACE_TILE .ARG1, ___bank_tileset_water_anim, _tileset_water_anim, .ARG0, 4
VM_POP          2
```

| Parameter | Meaning |
|---|---|
| `TARGET_TILE_IDX` | Variable holding the first VRAM slot to overwrite. Bit 11 (`0x0800`) selects VRAM bank 1 on Game Boy Color. |
| `TILEDATA_BANK` / `TILEDATA` | Bank and address of the source tileset — `___bank_<symbol>` and `_<symbol>`, where `<symbol>` is the tileset asset's symbol as shown in its asset settings. |
| `START_IDX` | Variable holding the first tile to read inside that tileset. |
| `LEN` | Number of consecutive tiles to copy. An immediate, not a variable. |

To advance frames, point `TARGET_TILE_IDX` / `START_IDX` at real script variables
(`VM_SET_CONST`, or a GB Studio variable) instead of pushed constants and update them
between calls.

### Technique 2 — Swap the metatile ID (per-position animation)

Technique 1 animates a tile everywhere it appears on screen at once. When a *single*
position has to animate on its own — a lone flower, one torch — swap the metatile
instead of the pixels: define one metatile per animation frame in the metatile scene,
then cycle them with [Replace meta tile](#replace-meta-tile) at that coordinate, with
**Commit render** checked so the change reaches the screen.

This costs one metatile ID per frame and only redraws the cells actually named, so it
suits a handful of animated positions; for a whole map's worth of water, use
Technique 1.

---

## Media

https://github.com/user-attachments/assets/854163c6-284a-4cfd-9dc4-24ba58504804

![image](https://github.com/user-attachments/assets/7fb07219-327f-4818-ba40-7e9a12484f4f)

https://github.com/user-attachments/assets/26751480-4c02-45ea-af46-df208dc619e9

![image](https://github.com/user-attachments/assets/61145b99-31a3-4ed2-912f-bbd7e786c066)

https://github.com/user-attachments/assets/72537786-be55-4dd1-968d-01b5c69c12fc

![image](https://github.com/user-attachments/assets/f6491b28-919a-4043-999f-effef4ac3023)

![SceneRendering3](https://github.com/user-attachments/assets/570cead9-04eb-4df7-8af4-e04235fbccb2)

---

<!-- SETTINGCOST:BEGIN -->
### What each engine setting costs

Every setting here changes what gets compiled. Figures are what you **get back by
turning the setting off**; rows marked *off by default* show what turning it **on**
costs instead, and sliders show the cost per step. A dash means that budget does not
move.

| Setting | Bank 0 | WRAM | Banked ROM |
|---|---|---|---|
| Size in pixels of one metatile → *16px* | +137 B | — | +747 B |
| Minimum metatile index to start checking for the entered metatile event *(slider 0–255, default 1)* | — | — | — |
| Enter metatile event detection mode → *Origin point* | — | — | −367 B |
| Platformer: Enable enter metatile event *(off by default — cost of turning it on)* | — | — | +16 B |
| Platformer: Enable collision metatile event *(off by default — cost of turning it on)* | — | — | — |
| Platformer: Enable down collision metatile event *(off by default — cost of turning it on)* | — | — | +23 B |
| Platformer: Enable right collision metatile event *(off by default — cost of turning it on)* | — | — | +29 B |
| Platformer: Enable up collision metatile event *(off by default — cost of turning it on)* | — | — | +23 B |
| Platformer: Enable left collision metatile event *(off by default — cost of turning it on)* | — | — | +29 B |
| Point N Click: Enable enter metatile event *(off by default — cost of turning it on)* | — | — | +18 B |
| Adventure: Enable enter metatile event *(off by default — cost of turning it on)* | — | — | +12 B |
| Adventure: Enable collision metatile event *(off by default — cost of turning it on)* | — | — | — |
| Adventure: Enable down collision metatile event *(off by default — cost of turning it on)* | — | — | −35 B |
| Adventure: Enable right collision metatile event *(off by default — cost of turning it on)* | — | — | +37 B |
| Adventure: Enable up collision metatile event *(off by default — cost of turning it on)* | — | — | +149 B |
| Adventure: Enable left collision metatile event *(off by default — cost of turning it on)* | — | — | +34 B |
| Shmup: Enable enter metatile event *(off by default — cost of turning it on)* | — | — | +19 B |
| Shmup: Enable collision metatile event *(off by default — cost of turning it on)* | — | — | — |
| Shmup: Enable down collision metatile event *(off by default — cost of turning it on)* | — | — | −18 B |
| Shmup: Enable right collision metatile event *(off by default — cost of turning it on)* | — | — | −24 B |
| Shmup: Enable up collision metatile event *(off by default — cost of turning it on)* | — | — | +37 B |
| Shmup: Enable left collision metatile event *(off by default — cost of turning it on)* | — | — | +58 B |
| Top Down: Enable enter metatile event *(off by default — cost of turning it on)* | — | — | +17 B |
| Top Down: Enable collision metatile event *(off by default — cost of turning it on)* | — | — | — |
| Top Down: Enable down collision metatile event *(off by default — cost of turning it on)* | — | — | +23 B |
| Top Down: Enable right collision metatile event *(off by default — cost of turning it on)* | — | — | +24 B |
| Top Down: Enable up collision metatile event *(off by default — cost of turning it on)* | — | — | +25 B |
| Top Down: Enable left collision metatile event *(off by default — cost of turning it on)* | — | — | +24 B |

- **Minimum metatile index to start checking for the entered metatile event**: going from 0 to 255 moves banked ROM by +24 B.

- **Platformer: Enable down collision metatile event** only applies when *Enable collision metatile event* is enabled.
- **Platformer: Enable right collision metatile event** only applies when *Enable collision metatile event* is enabled.
- **Platformer: Enable up collision metatile event** only applies when *Enable collision metatile event* is enabled.
- **Platformer: Enable left collision metatile event** only applies when *Enable collision metatile event* is enabled.
- **Adventure: Enable down collision metatile event** only applies when *Enable collision metatile event* is enabled.
- **Adventure: Enable right collision metatile event** only applies when *Enable collision metatile event* is enabled.
- **Adventure: Enable up collision metatile event** only applies when *Enable collision metatile event* is enabled.
- **Adventure: Enable left collision metatile event** only applies when *Enable collision metatile event* is enabled.
- **Shmup: Enable down collision metatile event** only applies when *Enable collision metatile event* is enabled.
- **Shmup: Enable right collision metatile event** only applies when *Enable collision metatile event* is enabled.
- **Shmup: Enable up collision metatile event** only applies when *Enable collision metatile event* is enabled.
- **Shmup: Enable left collision metatile event** only applies when *Enable collision metatile event* is enabled.
- **Top Down: Enable down collision metatile event** only applies when *Enable collision metatile event* is enabled.
- **Top Down: Enable right collision metatile event** only applies when *Enable collision metatile event* is enabled.
- **Top Down: Enable up collision metatile event** only applies when *Enable collision metatile event* is enabled.
- **Top Down: Enable left collision metatile event** only applies when *Enable collision metatile event* is enabled.

<details><summary>How these were measured</summary>

GB Studio 4.3.0-e1. This plugin's `engine/src/**/*.c` was compiled with the
toolchain and flags GB Studio itself uses (`lcc -msm83:gb -Wf--max-allocs-per-node 3000
-DHUGE_TRACKER -DRUMBLE_ENABLE=0x08u`) against a merged include tree, and the SDCC object
files' area records were read: `_HOME` is bank 0, `_DATA`/`_INITIALIZED`/`_BSS` are WRAM,
and `_CODE*`/`_CONST`/`_LIT`/`_INITIALIZER` are banked ROM.

Two caveats. Only this plugin's own engine sources are measured, so a setting that also
changes a struct shared with stock engine files can move a few more bytes in files the
plugin does not ship. And each setting is toggled on its own: a handful measure slightly
*negative* because enabling their code lets the compiler drop a fallback path elsewhere,
and settings that gate other settings only show their own contribution.

</details>
<!-- SETTINGCOST:END -->

## Memory Footprint

Measured against the stock GB Studio **4.3.0-e1** engine by `measure_plugin_memory.js` (per-file SDCC compile with GB Studio's own build flags, at default engine settings; report of 2026-08-13). Figures are this plugin's *delta* versus stock — a file that replaces a stock engine file counts only the difference, which is why a plugin can come out negative. Using the plugin's events additionally compiles a few bytes of GBVM script per call into your project's script banks, on top of the fixed cost below.

| Budget | Cost |
|---|---|
| Bank 0 (HOME) | −27 bytes |
| WRAM | +54 bytes |
| Banked ROM | +3,952 bytes |

- **Bank 0:** the plugin *gives back* 27 bytes — its replacements for stock engine files compile smaller than the originals. See [Bank 0 (HOME) Usage](#bank-0-home-usage).
- **WRAM:** 54 bytes, almost all of it metatile state; the reworked scroll code gives part of it back.
- **Banked ROM:** 3,952 bytes at the default `METATILE_SIZE_8`, 22 of which land in stock files the plugin does not ship but which recompile differently because it overrides `collision.h`, `load_save.h` and `scroll.h`. Switching to 16px metatiles adds 747 bytes of banked ROM and 137 bytes of bank 0 — which turns the bank 0 saving into a net cost of 110 bytes.
- **Engine WRAM headroom:** a stock GB Studio 4.3.0 project leaves about **854 bytes** of WRAM free (usable engine WRAM is 7,776 bytes at 0xC0A0–0xDF00; the stock engine uses 6,922). With this plugin installed roughly **800 bytes** remain. That does not change with the number of global variables your project defines: the script memory array is a fixed 3,584 bytes at stock engine settings (VM_HEAP_SIZE + VM_MAX_CONTEXTS × VM_CONTEXT_STACK_SIZE = 768 + 16 × 64 words).
- **SRAM:** yes — the plugin claims the **entire 8 KiB of SRAM bank 0** (0xA000–0xBFFF) for the metatile map + collision cache (METATILE_SIZE_16: 0x1C00 map + 0x0400 collision; METATILE_SIZE_8: 0x1F00 map + 0x0100 collision). Game saves are relocated to SRAM banks 1–3, so a cartridge with at least 32 KiB SRAM is required if your game uses save slots.

---

<!-- BANK0:BEGIN -->
## Bank 0 (HOME) Usage

Bank 0 is the 16 KB non-switchable ROM bank that the GB Studio engine core,
the interrupt handlers and the GBDK runtime all share. Banked ROM is cheap
(add another bank), bank 0 is not, so it is usually the first thing a project
runs out of.

| | Bytes |
|---|---|
| Bank 0 used by this plugin | **−27** |
| Bank 0 free with this plugin installed | **1,478** of 16,384 (91% used) |

**This plugin gives bank 0 space back.** Its replacements for stock engine
files compile smaller than the originals, freeing 27 bytes.

| Module | This plugin | Stock engine | Bank 0 cost |
|---|---|---|---|
| `core/collision.c` | 339 | 401 | −62 |
| `core/scroll.c` | 321 | 286 | +35 |

Modules that replace or patch a stock engine file only cost the *difference*:
the stock version's bank 0 bytes were being spent anyway.

<details><summary>How this was measured</summary>

GB Studio 4.3.0-e1, default engine settings. Each module is compiled with the
toolchain and flags GB Studio itself uses, and the `A _HOME size` record SDCC
writes into the resulting `.rel` object is read back; the stock column is the
same compile of the engine file this module replaces.

The "free" figure is a stock project with this plugin and nothing else. Your
own number will differ: other plugins, and any engine settings that change what
the core compiles, move it independently of this plugin.

</details>
<!-- BANK0:END -->

## Changelog

Grouped by the date each change was merged into the official
[gb-studio-plugins](https://github.com/gb-studio-dev/gb-studio-plugins) repository.

Only bug fixes, new features and feature changes are listed. Engine version
bumps, patch regeneration, packaging fixes and documentation edits are omitted.

### 2026-07-27

- Added metatile enter detection modes.
- Fixed ScreenScroll / ContinuousScene plugin compatibility with the plugin's 8px mode.

### 2026-07-19

- Added a "Redraw Meta Tiles" event, to redraw a region of the screen that may have been modified directly in VRAM.

### 2026-06-28

- Added ContinuousScenePlugin compatibility.

### 2026-06-14

- Added custom script parameter / stack support to the events.

### 2026-06-12

- Fixed the metatile overlap event.
- Fixed the actor position property field in custom events.

### 2026-06-08

First published in the official plugin repository. This entry covers everything
developed since the plugin's standalone release in February 2025:

- Metatile16 release, followed by the 8px metatile mode.
- Added an "Attach a script to a metatile" event.
- Extended scene stack (SceneStackExPlugin) integration.
- Larger scene support, plus an engine setting for the metatile scene maximum size, and metatile scene size validation.
- Metatile collision editing, and a collision event for the 8px mode.
- Background cache, so metatiles are not regenerated when the same background is used in several scenes.
- Exposed additional engine fields.
- Load Metatile event compilation time optimisations, plus three rounds of runtime optimisation.
- Fixes: parallax, sprite glitch on transition, scene scroll rendering when moving north in a 20x18 metatile scene, tile updates on scene edges while scrolling, scrolling and changing scene between metatile and non-metatile scenes, metatile replacement during scene transitions and during screen scroll, metatile submapping in the ScreenScroll variant, projectile rendering, and the small blip when scrolling up with a HUD margin.
