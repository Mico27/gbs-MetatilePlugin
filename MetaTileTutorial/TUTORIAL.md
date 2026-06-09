# MetaTile Plugin — Step-by-Step Tutorial

**Plugin version:** 4.3.0 · **Requires GB Studio ≥ 4.3.0**

This tutorial walks you through building a complete top-down scene that uses the MetaTile Plugin.
By the end you will have:

- A shared tileset driving both a metatile scene and a playable scene.
- Tiles that can be replaced at runtime (destructible/interactive tiles).
- Collisions that can be toggled at runtime.
- A second metatile scene used to submap (stamp) a chunk of tiles into the active scene.
- An **Enter Metatile** event that changes the player's animation when they step onto grass.

The finished project is included in this folder (`MetaTileTutorial.gbsproj`) so you can open it and
compare your work at any point.

---

## Table of Contents

1. [Installing the Plugin](#1-installing-the-plugin)
2. [Understanding Metatiles](#2-understanding-metatiles)
3. [Setting Up the Metatile Scene](#3-setting-up-the-metatile-scene)
4. [Configuring Engine Settings](#4-configuring-engine-settings)
5. [Setting Up the Main Scene](#5-setting-up-the-main-scene)
6. [Defining Constants and Variables](#6-defining-constants-and-variables)
7. [Getting the Metatile in Front of the Player](#7-getting-the-metatile-in-front-of-the-player)
8. [Replacing Metatiles at Runtime](#8-replacing-metatiles-at-runtime)
9. [Working with Collisions](#9-working-with-collisions)
10. [Submapping Metatiles](#10-submapping-metatiles)
11. [Reacting to Metatile Events](#11-reacting-to-metatile-events)
12. [Running the Game](#12-running-the-game)
13. [Troubleshooting](#13-troubleshooting)

---

## 1. Installing the Plugin

### Opening the Plugin Manager

In GB Studio, open the **Plugin Manager** from the **Edit** menu (or press the plug icon in the
toolbar).

<img width="256" height="165" alt="Open_Plugin_Manager" src="https://github.com/user-attachments/assets/dfd1594c-3fef-44bf-a145-cfb3578a1eec" />

### Adding the Plugin to Your Project

Search for **MetaTile Plugin** and click **Add to Project**. GB Studio copies the plugin files into
your project's `plugins/` folder automatically.

<img width="588" height="698" alt="Add_Plugin_To_Project" src="https://github.com/user-attachments/assets/723205ac-968b-495f-ad8c-e2a53fb992c6" />

> **Tip — manual install:** If you prefer to install manually, copy the `MetaTilePlugin/` folder
> (found in this repository under `src/`) directly into your project's `plugins/` folder, then
> restart GB Studio.

---

## 2. Understanding Metatiles

A **metatile** is a logical grouping of one or more raw GB tiles treated as a single addressable
unit. Instead of storing a raw tile index for every cell of a large map, the plugin stores one
**metatile ID** per cell. Each ID points to a position inside a dedicated **metatile scene** that
defines what raw tile(s) that ID looks like.

### Two metatile sizes

| Mode | Footprint | Map data limit | Collision data limit |
|------|-----------|----------------|----------------------|
| **8px** | 1 × 1 raw tile (8 × 8 px) | 7 936 bytes | 256 bytes |
| **16px** | 2 × 2 raw tiles (16 × 16 px) | 7 168 bytes | 1 024 bytes |

This tutorial uses **16px** mode throughout. The 8px mode follows the same steps; the only
differences are noted where they arise.

### Why metatiles?

- **Smaller map data** — one ID per metatile cell instead of multiple raw tile indices.
- **Runtime tile replacement** — change a single ID in SRAM to swap out an entire 2 × 2 block of
  tiles anywhere on the map, at any time.
- **Persistent changes** — modifications live in SRAM bank 0 and are not wiped by save or load
  operations.

### SRAM layout

The plugin reserves SRAM bank 0 exclusively for metatile data:

```
0xA000  ┌─────────────────────────────┐
        │  Map data (metatile IDs)    │  16px: up to 7 168 bytes
        ├─────────────────────────────┤
        │  Collision data             │  16px: 1 024 bytes (4 sub-tiles × 256 IDs)
0xBFFF  └─────────────────────────────┘
```

SRAM banks 1–3 are used by GB Studio's save system and are never touched by this plugin.

---

## 3. Setting Up the Metatile Scene

The **metatile scene** is a dedicated GB Studio scene whose sole purpose is to define your
available metatiles. It is **never visited at runtime** — the compiler reads it only at build time
to create the metatile lookup table.

### Required dimensions

| Mode | Width | Height |
|------|-------|--------|
| **8px** | 128 px (16 tile columns) | any |
| **16px** | **256 px (32 tile columns / 16 metatile columns)** | any |

For 16px mode each 2 × 2 block of tiles defines one metatile ID. Up to 256 metatiles can be
defined (IDs 0–255).

### Creating the metatile scene

1. Create a new scene and name it something like **Metatile Scene**.
2. Assign a background image that is exactly **256 pixels wide** for 16px mode. This image must be
   built from the same shared tileset as your playable scenes.
3. **Set the scene's tileset** (in the scene properties panel). Any scene that uses this metatile
   scene — via **Load meta tiles** or **Submap metatiles** — must use the **same tileset** as this
   metatile scene. Using a different tileset will cause the tile-index lookup to produce wrong
   results at compile time. Scenes that use a different metatile scene can use a different tileset.
4. Set the scene type to any non-playable type (Top-Down is fine — the scene is never entered).

<img width="712" height="474" alt="Metatile_Scene_Setup" src="https://github.com/user-attachments/assets/865493a4-535d-41a8-a592-351198829d76" />

4. Paint collision data on the metatile scene. This is the collision data that will be copied to
   SRAM when the scene is initialised at runtime — so paint it carefully.

<img width="403" height="348" alt="Set_Metatile_Scene_Collisions" src="https://github.com/user-attachments/assets/e1b8553f-65fe-4877-a41f-767f07502a72" />

> **ID assignment:** IDs are assigned left-to-right, top-to-bottom, two-tile column by two-tile
> column. The leftmost 2 × 2 block (columns 0–1, rows 0–1) is ID **0**, the next block (columns
> 2–3, rows 0–1) is ID **1**, and so on. If your metatile scene is taller than 16 metatile rows,
> IDs continue onto the next row of blocks.

### Designing metatiles with Tiled (optional)

The `Tiled/` subfolder in this tutorial project contains a ready-made Tiled project
(`tutorial.tiled-project`) with the `forest_metatiles.tsx` tileset already configured. Using Tiled
to design your metatile scene can make it easier to visualise tile boundaries. Export the result as
a PNG and import it into GB Studio as a background image.

---

## 4. Configuring Engine Settings

Open **Settings → Engine** in GB Studio to configure the plugin's global and per-scene-type
options.

### Global metatile settings

<img width="950" height="481" alt="Metatile_settings" src="https://github.com/user-attachments/assets/9ff05186-c067-40bc-a52d-69789c659bb7" />

| Setting | Description |
|---------|-------------|
| **Size in pixels of one metatile** | Set to `16px` for this tutorial. Applies to the entire project. |
| **Minimum metatile index to start checking for the entered metatile event** | Metatile IDs below this value are ignored by the Enter event. Leave at `1` to skip ID 0 (the "empty" tile). |
| **Minimum metatile index — down / right / up / left collision event** | Same threshold per collision direction. |

### Per scene-type flags

Each scene type has its own group of enable flags. Expand the group for the types you use and
enable only what you need — each enabled flag adds a small per-frame check.

<img width="744" height="481" alt="Metatile_Scene_Type_settings" src="https://github.com/user-attachments/assets/bf69550d-a3a4-4f96-93e5-0cae18a3b507" />

For this tutorial, expand **Top-Down** and enable:

- **Enable enter metatile event** — so we can change the player's animation on grass tiles.

> You do not need to enable any collision event flags for the examples in this tutorial. They can be
> enabled later when you need them.

---

## 5. Setting Up the Main Scene

Create your playable scene and assign it the **same shared tileset** as the metatile scene.

<img width="793" height="883" alt="Main_Scene_Setup" src="https://github.com/user-attachments/assets/071e0e41-4896-4674-a1b9-50953739dee4" />

Key points:

- The scene type must match the engine setting flag you just enabled (**Top-Down** here).
- **Tileset must match the metatile scene** — the main scene must use the same tileset as the
  metatile scene it references in **Load meta tiles**. Set the tileset in the scene properties
  panel before painting any tiles. Other scenes that use a different metatile scene can use a
  different tileset.
- The scene's tilemap will be **rewritten at compile time** by the **Load meta tiles** event — every
  2 × 2 tile block is replaced with its corresponding metatile ID. You do **not** need to paint any
  collision data on the main scene; collision comes entirely from the metatile scene's SRAM data.

### Adding the Load meta tiles event

In the scene's **On Init** script, add a **Load meta tiles** event (found under **Meta Tiles** in
the event picker) as the very first event:

| Field | Value |
|-------|-------|
| Metatile Scene | *Your metatile scene* |
| Must match metatile color attributes | ☐ (leave unchecked unless you need palette-differentiated metatiles) |
| Must match metatile collision | ☐ (leave unchecked for this tutorial) |

At compile time the compiler scans every 2 × 2 block in the main scene, looks it up in the
metatile scene, and replaces the tilemap with an array of metatile IDs. At runtime the event copies
the collision table and ID map from ROM into SRAM.

> **Compile error — tile not found:** If a tile group in the main scene does not appear in the
> metatile scene, the compiler throws an error with the exact map coordinates. Add the missing
> pattern to the metatile scene and recompile.

---

## 6. Defining Constants and Variables

Using named constants for metatile IDs keeps your scripts readable and easy to maintain. Open the
**Constants** panel and add one constant for each metatile ID your scripts need to reference.

<img width="206" height="418" alt="Constants_And_Variables" src="https://github.com/user-attachments/assets/de81b752-6bbb-4346-8638-3802df5518fb" />

For this tutorial the project uses:

| Constant | Value | Meaning |
|----------|-------|---------|
| `GROUND_METATILE_ID` | `1` | Bare ground tile |
| `SHORTGRASS_METATILE_ID` | `6` | Short grass tile |
| `GRASS_METATILE_ID` | `16` | Tall grass tile |
| `DIR_DOWN` | `0` | Direction constant |
| `DIR_RIGHT` | `1` | Direction constant |
| `DIR_UP` | `2` | Direction constant |
| `DIR_LEFT` | `3` | Direction constant |

And the following **variables**:

| Variable | Purpose |
|----------|---------|
| `FrontMetatileId` | Stores the ID of the metatile in front of the player |
| `TileX` | Tile X coordinate of the cell in front of the player |
| `TileY` | Tile Y coordinate of the cell in front of the player |
| `PlayerDirection` | Tracks which direction the player is facing |
| `IsOnGrass` | Flag: 1 if player is currently on a grass metatile |
| `AnimationThread` | Thread ID of the player's extra animation script |

> **How to find a metatile ID:** In the metatile scene, count 2 × 2 blocks from the top-left,
> left-to-right and top-to-bottom. The block at column `c`, row `r` has ID
> `r × 16 + c` (for a 256 px wide scene with 16 metatile columns per row).
> You can also use **Get meta tile at position** at runtime to read back the ID at any coordinate.

<img width="208" height="187" alt="Add_Metatile_Id_Constant" src="https://github.com/user-attachments/assets/1a7183fd-1b98-43a7-8873-f33b2d02f8f1" />

---

## 7. Getting the Metatile in Front of the Player

A common pattern is to read the metatile ID at the tile immediately in front of the player (e.g.
to decide what happens when the player presses A).

The approach:

1. Read the player's current tile position using the **Actor tile position** actor fields.
2. Offset the tile position by one in the direction the player is facing to get the target
   coordinate.
3. Use **Get meta tile at position** to read the ID at that coordinate into `FrontMetatileId`.

<img width="738" height="1055" alt="Get_Metatile_In_Front_Of_Player" src="https://github.com/user-attachments/assets/ce6f08b7-70f8-4814-b9b0-2110dcdedaeb" />

**Get meta tile at position** — parameter summary:

| Field | Description |
|-------|-------------|
| X | Tile column (for 16px mode this is the raw tile column, **not** the metatile column). |
| Y | Tile row. |
| Variable | Variable to receive the metatile ID (0–255). |

> **16px coordinates:** In 16px mode each metatile spans 2 raw tiles. The position fields on all
> Meta Tiles events use **raw tile** units, not metatile units. To convert: metatile column = raw
> tile column ÷ 2.

Once you have the ID in `FrontMetatileId` you can branch on it with an **If Variable Equals**
event, comparing against your named constants.

---

## 8. Replacing Metatiles at Runtime

**Replace meta tile** overwrites the metatile ID stored in SRAM at a given position and optionally
redraws the screen at that location.

<img width="716" height="491" alt="Replacing_Metatile" src="https://github.com/user-attachments/assets/7fdf55b2-6786-4446-83a2-aa1fb1f1f92a" />

| Field | Description |
|-------|-------------|
| X | Raw tile column of the target cell. |
| Y | Raw tile row of the target cell. |
| Metatile Id | The new metatile ID to store. |
| Commit render | Check when the tile is currently **on-screen** to redraw it immediately. Leave unchecked for off-screen tiles. |

### Typical use-cases

- **Destructible tiles:** After the player interacts with a tile (e.g. cutting grass), call
  **Replace meta tile** with `GROUND_METATILE_ID` at the tile's position to swap the grass tile for
  bare ground.
- **Opening doors / bridges:** Change a wall tile to a floor tile when a condition is met.
- **Dynamic environment:** Flood a room with water tiles when a switch is triggered.

### Restoring a tile — Reset meta tile

**Reset meta tile** restores a position to the metatile ID that was originally loaded from ROM at
scene init, undoing any runtime replacement:

| Field | Description |
|-------|-------------|
| X / Y | Raw tile coordinate. |
| Commit render | Same behaviour as Replace meta tile. |

> **Off-screen writes:** When replacing tiles that are off-screen at the time of the call, always
> leave **Commit render** unchecked. Attempting to write to VRAM for off-screen positions causes
> visual corruption. The tile will appear correctly the next time it scrolls into view.

---

## 9. Working with Collisions

Collision data lives in SRAM alongside the metatile IDs. You can change the collision for a given
metatile ID globally — affecting every tile on the map that uses that ID.

### Setting collisions on the metatile scene

The easiest way to set initial collisions is to paint them directly on the **metatile scene** in
the GB Studio editor. The plugin copies this data to SRAM when **Load meta tiles** runs.

<img width="403" height="348" alt="Set_Metatile_Scene_Collisions" src="https://github.com/user-attachments/assets/bffeac07-a1fe-46b4-bcf8-2520abc387fd" />

### Replacing collision at runtime

Use **Replace collision (16px metatile)** to change the collision bytes for all four sub-tiles of a
metatile ID at once:

<img width="681" height="208" alt="Replace_Collision" src="https://github.com/user-attachments/assets/b53857f0-27e0-4d9a-bd5b-61d38a5ee314" />

| Field | Description |
|-------|-------------|
| Metatile Id | The metatile ID whose collision to change (0–255). |
| Collision Top Left | New collision byte for the top-left sub-tile of the metatile. |
| Collision Top Right | New collision byte for the top-right sub-tile. |
| Collision Bottom Left | New collision byte for the bottom-left sub-tile. |
| Collision Bottom Right | New collision byte for the bottom-right sub-tile. |

For **8px mode** use **Replace collision (8px metatile)** instead, which takes a single collision
byte:

| Field | Description |
|-------|-------------|
| Metatile Id | The metatile ID to change. |
| Collision | New collision byte. |

### Important — collision change is global

**Replace collision** modifies the collision for a metatile **ID**, not for a single position on
the map. Every tile that uses that ID will have its collision changed simultaneously. If you need
different collisions at different positions for what looks like the same tile, you must use
distinct metatile IDs (i.e. place two visually identical but separately indexed tiles in the
metatile scene).

### Use-cases

- **Phase-through platforms:** Clear the bottom collision of a tile so the player can jump through
  it from below.
- **Triggers without walls:** Set a tile's collision bytes to `0` so the player can walk through
  while the Enter event still fires.
- **Lava / ice zones:** Switch collision properties based on a game-state condition.

---

## 10. Submapping Metatiles

**Submap metatiles** copies a rectangular block of metatile IDs from a source scene's ROM data into
the active scene's SRAM. This is the primary tool for loading new chunks of map content at runtime
— for example, changing a room layout, revealing a hidden area, or assembling a procedural map
from pre-authored pieces.

### The submap scene

Create one or more extra scenes that act as "tile libraries". These scenes are never visited; they
exist purely to store alternate arrangements of metatile IDs.

> **Tileset requirement:** A submap scene must use the **same tileset as the metatile scene** that
> the active scene loaded with **Load meta tiles**. Set the tileset in the scene properties panel
> before painting any tiles. A mismatched tileset will cause wrong tile indices to be looked up at
> compile time, resulting in garbled graphics at runtime.

<img width="621" height="768" alt="Metatile_Submap_Scene" src="https://github.com/user-attachments/assets/12b028c5-672d-470c-a56e-2073a26a06fa" />

### Submapping from a different metatile layout scene

Use a separate scene (different background from the metatile scene) to store alternate map chunks.
Point **Submap metatiles** at that scene to stamp its region into the active scene.

<img width="1140" height="1300" alt="Submapping_From_Another_Metatile_Scene" src="https://github.com/user-attachments/assets/35e7eec9-8d21-473d-a3c9-7c9cb7369104" />

### Submapping from the same metatile scene

You can also re-use the metatile scene itself as a stamp source. Because every possible metatile
pattern already lives there, you can use a region of the metatile scene as a ready-made tile chunk.

<img width="1130" height="1286" alt="Submapping_From_Same_Metatile_Scene" src="https://github.com/user-attachments/assets/751de089-9d4d-4ac6-9622-005a8c0e7df8" />

### Event parameters

| Field | Description |
|-------|-------------|
| Scene | The source scene to copy metatile IDs from. |
| Source X / Source Y | Top-left **raw tile** coordinate in the source scene. |
| Destination X / Destination Y | Top-left **raw tile** coordinate in the active scene to paste into. |
| Width / Height | Size of the region in **raw tile** units (maximum 31 per axis). |
| Commit render | Check to redraw each copied row immediately (only use for on-screen regions). |

### Use-cases

- **Room transitions:** Before the player enters a new area, call Submap to load the new layout
  into the active scene's SRAM without switching scenes.
- **Destructible sections:** Replace a destructible wall section with rubble tiles in one call
  instead of many individual Replace meta tile calls.
- **Dynamic world assembly:** Assemble a large world map from authored chunk scenes at startup.

---

## 11. Reacting to Metatile Events

The plugin can fire a custom script whenever the player enters a new metatile or collides with one
from a specific direction. This section demonstrates the **Enter** event using the player animation
example from the tutorial project.

### Configuring the player sprite

The player sprite for this tutorial has two animation groups:

- **Default** — the standard idle and walk animations for all four directions.
- **OnGrass** — a modified walk cycle to play when the player is on grass.

<img width="189" height="327" alt="Set_Player_Sprite_Animation_States" src="https://github.com/user-attachments/assets/229b656c-46cc-4c34-affb-6d4942ed576a" />

The player sprites in the onGrass animation state has the bottom 8px tiles drawn behind the background
to achieve the walking on grass effect, so sprites should be set to **8 × 8 px** tile mode for this.

<img width="655" height="168" alt="Set_Sprite_8x8" src="https://github.com/user-attachments/assets/10a983f8-009e-4a83-becf-c536673a294d" />

### Attaching the Enter Metatile event

In the scene's **On Init** script, after **Load meta tiles**, add an **Attach a Script to a
Metatile Event** event:

<img width="741" height="1169" alt="Attach_Metatile_Enter_Event" src="https://github.com/user-attachments/assets/3073ea6c-f10f-4462-8d8c-d7f840584c7c" />

| Field | Value |
|-------|-------|
| Select Metatile Event | **Metatile Enter** |
| On Metatile Event | *(your handler script — see below)* |

### The handler script

When the Enter event fires the engine populates these read-only engine fields:

| Engine Field | Description |
|--------------|-------------|
| `overlap_metatile_id` | Metatile ID of the tile the player just entered. |
| `overlap_metatile_x` | Raw tile X of that metatile. |
| `overlap_metatile_y` | Raw tile Y of that metatile. |

A typical handler for the grass animation example:

1. If **Entered Metatile Id** (engine field `overlap_metatile_id`) equals `GRASS_METATILE_ID` and `IsOnGrass` equals `0`:
   - Set `IsOnGrass` to `1`.
   - Switch the player's animation group to **OnGrass**.
2. Else if `IsOnGrass` equals `1` (player was on grass and is now leaving):
   - Set `IsOnGrass` to `0`.
   - Switch the player's animation group back to **Default**.

### Available event types

| Event type | When it fires |
|------------|---------------|
| **Metatile Enter** | Player bounding box moves onto a new metatile (ID ≥ minimum threshold). |
| **Metatile Down Collision** | Player is blocked moving downward into a tile. |
| **Metatile Right Collision** | Player is blocked moving rightward. |
| **Metatile Up Collision** | Player is blocked moving upward. |
| **Metatile Left Collision** | Player is blocked moving leftward. |
| **Metatile Any Collision** | Any of the four directional collisions. |

For collision events the engine populates:

| Engine Field | Description |
|--------------|-------------|
| `collided_metatile_id` | Metatile ID of the blocking tile. |
| `collided_metatile_x` | Raw tile X of the blocking tile. |
| `collided_metatile_y` | Raw tile Y of the blocking tile. |
| `collided_metatile_dir` | Collision direction (matches GB Studio direction constants). |
| `collided_metatile_source` | Internal source flag. |

### Removing an event handler

Use **Remove a Script from Metatile Event** to unregister a handler, for example when the player
leaves a scene or when a game-state change means the script should no longer fire:

| Field | Value |
|-------|-------|
| Select Metatile Event | The event slot to clear. |

> **One handler per slot:** Each event type has a single script slot. Attaching a new script to an
> already-occupied slot replaces the previous one without error.

---

## 12. Running the Game

Click **Play** in GB Studio to run the game in the built-in emulator. With the tutorial project
open you should see:

- The player walking through the forest scene.
- The player's animation switching when entering and leaving short-grass tiles.
- The ability to interact with grass tiles in front of the player (press **A**) to replace them.

https://github.com/user-attachments/assets/a9cf412e-b363-455f-bbd7-eadcf858e870

---

## 13. Troubleshooting

### "Tile pattern not found" compile error

The compiler could not match a 2 × 2 tile block in the main scene to any metatile in the metatile
scene. The error message includes the tile coordinate.

**Fix:** Open the metatile scene and add the missing tile pattern. Every tile combination present
in a main scene must have a corresponding entry in the metatile scene.

### Map data too large

The scene exceeds the SRAM size limit (7 168 bytes for 16px mode).

**Fix:** Reduce the scene dimensions, or redesign the map to fit within the limit. Use the formula:
`next_power_of_2(width_in_metatiles) × height_in_metatiles ≤ 7 168`.

### Metatile events not firing

Check that the relevant per-scene-type flags are enabled in **Settings → Engine → Metatiles**. Each
scene type has its own set of enable flags and they are all off by default.

Also verify that the **Minimum metatile index** threshold is not set higher than the IDs you are
using — IDs below the threshold are silently ignored by the event system.

### Visual corruption after Replace meta tile

You attempted to write to a tile that was off-screen at the time. Uncheck **Commit render** on any
Replace / Submap call where the target position is not currently visible. The tile will update
automatically the next time it scrolls into view.

### Collision changes not taking effect

**Replace collision** edits the collision for a metatile **ID**, not a map position. If multiple
tiles with the same ID exist and you only want to change one, you need distinct IDs — add a
duplicate metatile entry to the metatile scene with different collision data and use
**Replace meta tile** to assign that new ID to the specific position first.

### Metatile changes lost after scene reload

SRAM bank 0 (metatile data) is preserved across save/load operations, but it **is** reset when
**Load meta tiles** runs again (e.g. when re-entering the scene). If you need runtime changes to
survive a scene transition, store the relevant IDs in variables before leaving and reapply them
with **Replace meta tile** events after **Load meta tiles** runs on re-entry.

---

## Summary of All Meta Tiles Events

| Event | Event ID | What it does |
|-------|----------|-------------|
| **Load meta tiles** | `EVENT_LOAD_META_TILES` | Core setup — rewrites tilemap at compile time, loads metatile data to SRAM at runtime. |
| **Get meta tile at position** | `EVENT_GET_META_TILE_AT_POS` | Reads metatile ID from SRAM at a tile position into a variable. |
| **Get meta tile collision at position** | `EVENT_GET_META_TILE_COLLISION_AT_POS` | Reads the collision byte at a tile position from SRAM into a variable. |
| **Replace meta tile** | `EVENT_REPLACE_META_TILE` | Overwrites metatile ID in SRAM; optionally redraws the tile. |
| **Reset meta tile** | `EVENT_RESET_META_TILE` | Restores a position to its original ROM metatile ID. |
| **Replace collision (16px)** | `EVENT_REPLACE_COLLISION_16` | Changes collision bytes for all four sub-tiles of a 16px metatile ID. |
| **Replace collision (8px)** | `EVENT_REPLACE_COLLISION_8` | Changes the collision byte for an 8px metatile ID. |
| **Submap metatiles** | `EVENT_SUBMAP_METATILES` | Copies a rectangular region of metatile IDs from a source scene into the active scene's SRAM. |
| **Attach a Script to a Metatile Event** | `PM_EVENT_METATILE_SCRIPT` | Registers a script to run when the selected metatile event fires. |
| **Remove a Script from Metatile Event** | `PM_EVENT_METATILE_SCRIPT_CLEAR` | Unregisters the script from the selected metatile event slot. |
