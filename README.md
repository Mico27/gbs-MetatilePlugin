# gbs-MetatilePlugin

**Version 4.3.0. Requires GB Studio 4.3.0 or newer.**

Builds scenes out of reusable blocks instead of individual tiles, and keeps the whole map in the cartridge's memory so a script can change any part of it at any time.

That buys three things. Maps get much bigger, because a 16 by 16 block costs one number instead of four tiles. Changes stick: a bridge you build, a wall you blow up or a crop you plant stays that way. And a script can edit parts of the map that are nowhere near the screen, so a whole level can be generated or rearranged while the player is elsewhere.

Example projects ship for the 8 pixel and 16 pixel block sizes, and for 16 pixel blocks combined with screen scrolling.

---

## Table of Contents

1. [Concepts](#concepts)
2. [Project Setup](#project-setup)
3. [Engine Settings](#engine-settings)
4. [Size Limits and Restrictions](#size-limits-and-restrictions)
5. [Events Reference](#events-reference)
6. [Engine Fields Reference](#engine-fields-reference)
7. [Animating Tiles](#animating-tiles)
8. [FAQ](#faq)
9. [Media](#media)
10. [Memory Footprint](#memory-footprint)
11. [Bank 0 (HOME) Usage](#bank-0-home-usage)
12. [Changelog](#changelog)

---

## Concepts

### What is a Metatile?

A metatile is a group of one or more ordinary tiles treated as one piece. Rather than storing a tile for every cell of a large map, the plugin stores one metatile number per cell, and each number points into a **metatile scene** that says what it looks like. The map data shrinks, and changing a large area of the scene means changing a handful of numbers.

### Two Metatile Sizes

Two sizes, chosen for the whole project with the **Size in pixels of one metatile** setting:

| Mode | Metatile footprint | Map data limit | Collision data limit |
|------|--------------------|----------------|----------------------|
| **8px** | 1 tile | 7,936 bytes | 256 bytes |
| **16px** | 2 by 2 tiles | 7,168 bytes | 1,024 bytes |

At **8px** a metatile is one tile, so you can change any single tile. At **16px** a metatile covers a 2 by 2 block, which halves the map's resolution and lets each block be built from four different tiles. 16px is the usual choice for a large overworld, and 8px for a scene where individual tiles matter.

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

Game saves live in banks 1 to 3 and leave bank 0 alone, so a save or a load does not wipe your map changes. They last until the scene is loaded again.

---

## Project Setup

### 1. Create a Common Tileset

Draw one tileset image holding every tile your project needs. The metatile scene and every scene using metatiles must share it, so that a tile number means the same thing everywhere.

### 2. Create a Metatile Scene

The metatile scene exists only to define the blocks. The player never visits it, and the build reads it to work out what each metatile number looks like.

- **8px mode**: The metatile scene must be exactly **128 pixels wide** (16 tiles). Each individual tile in the scene is one metatile. Up to 256 metatiles can be defined (tiles 0–255).
- **16px mode**: The metatile scene must be exactly **256 pixels wide** (32 raw tiles / 16 metatile columns). Each 2×2 block of tiles is one metatile. Up to 256 metatiles can be defined.

Paint the metatile scene with the shared tileset and set its collision. That collision is what the game uses for every scene built from these blocks, so a wall block is solid everywhere it appears.

### 3. Add Load Meta Tiles to the Main Scene

In the **On Init** script of each scene that should use metatiles, add a **Load meta tiles** event and point it at your metatile scene.

When the project is compiled, the compiler rewrites the scene's tilemap: every tile group (1×1 for 8px, 2×2 for 16px) is looked up in the metatile scene and replaced by its corresponding metatile ID. The result is a compact array of IDs that is stored in ROM and copied into SRAM at runtime.

**Optional matching flags** on the Load meta tiles event:
- **Must match metatile color attributes** keeps blocks apart when they use the same tiles in different colours.
- **Must match metatile collision** keeps blocks apart when they look the same but have different collision.

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
| **Minimum metatile index, down collision event** | 1 | The same threshold for the Down collision event. |
| **Minimum metatile index, right collision event** | 1 | The same for Right. |
| **Minimum metatile index, up collision event** | 1 | The same for Up. |
| **Minimum metatile index, left collision event** | 1 | The same for Left. |

### Per Scene-Type Flags

Platformer, Top-Down, Adventure, Point and Click and Shmup each have their own tickboxes. None are on by default. Turn on only what your game uses, since each one costs time every frame.

| Flag | Description |
|------|-------------|
| **Enable enter metatile event** | Fires when the player's bounding box transitions onto a new metatile. |
| **Enable collision metatile event** | Master switch for the directional collision events below. |
| **Enable down collision metatile event** | Fires when the player is blocked moving down. |
| **Enable right collision metatile event** | Fires when the player is blocked moving right. |
| **Enable up collision metatile event** | Fires when the player is blocked moving up. |
| **Enable left collision metatile event** | Fires when the player is blocked moving left. |

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

Several events have a **Commit render** tickbox. Unticked, the map is changed but the screen is not redrawn. Leave it unticked for anything off screen, because redrawing a position that is not visible corrupts the display. Tick it only for positions the player can see.

### SRAM Bank 0 is Reserved

Nothing else may use save memory bank 0. The plugin writes the whole map and its collision there when a scene starts, and again whenever a script changes the map.

### Save/Load Compatibility

Game saves use banks 1 to 3. The map in bank 0 is not part of a save, so it survives a load but is not carried between play sessions.

---

## Events Reference

All events appear under the **Meta Tiles** group in the script editor.

---

### Load meta tiles

The setup event. It has to run in a scene's init script before any other metatile event.

During the build it converts the scene's map from tiles into metatile numbers. When the scene starts it copies that map and its collision into save memory bank 0 and redraws the screen from it.

| Field | Description |
|-------|-------------|
| Metatile Scene | The scene that defines the blocks. |
| Must match metatile color attributes | Treat blocks with different colours as different blocks. |
| Must match metatile collision | Treat blocks with different collision as different blocks. |

<img width="739" height="130" alt="image" src="https://github.com/user-attachments/assets/aad82d0e-f5a5-44b6-a0ea-b2d042aa8ea1" />

---

### Get meta tile at position

Reads the metatile number at a position into a variable. The position is in tiles, not pixels.

| Field | Description |
|-------|-------------|
| X | Horizontal tile position (8px: tile column; 16px: tile column, not metatile column). |
| Y | Vertical tile position. |
| Variable | Destination variable to receive the metatile ID (0–255). |

<img width="743" height="91" alt="image" src="https://github.com/user-attachments/assets/0db265a2-d7a4-4a9b-8e82-ab56b220aa27" />

---

### Get meta tile collision at position

Reads the collision byte for the tile at the given position from SRAM collision data and writes it into a variable. For 16px metatiles, the sub-tile within the 2×2 block is determined by the parity of X and Y.

| Field | Description |
|-------|-------------|
| X | Horizontal tile position. |
| Y | Vertical tile position. |
| Variable | Destination variable to receive the collision byte. |

<img width="742" height="95" alt="image" src="https://github.com/user-attachments/assets/d4ed93ae-e6e9-4b93-a323-17c940d9db50" />

---

### Replace meta tile

Shown in scripts as *Assign meta tiles*.

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

Shown in scripts as *Reset meta tiles*.

Restores the metatile at the given position to the value that was originally loaded from ROM (i.e. the value set at scene init by Load meta tiles).

| Field | Description |
|-------|-------------|
| X | Horizontal tile position. |
| Y | Vertical tile position. |
| Commit render | Check to immediately update the on-screen tile graphics. Leave unchecked for off-screen positions. |

<img width="741" height="97" alt="image" src="https://github.com/user-attachments/assets/a33d93e8-d13a-4df1-8b80-2ba926d8d0ff" />

---

### Replace collision (16px metatile)

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

Overwrites the collision byte for a single 8px metatile ID in SRAM.

| Field | Description |
|-------|-------------|
| Metatile Id | The metatile ID whose collision data to change (0–255). |
| Collision | New collision byte. |

---

### Submap metatiles

Shown in scripts as *Submap metatiles*.

Copies a rectangular region of metatile IDs from another scene's ROM tilemap directly into the active scene's SRAM map data, and optionally re-renders the affected rows on screen. This is useful for room transitions, destructible terrain sections, or procedural map assembly.

| Field | Description |
|-------|-------------|
| Scene | The source scene to copy metatile IDs from. |
| Source X / Source Y | Top-left tile coordinate in the source scene. |
| Destination X / Destination Y | Top-left tile coordinate in the active scene to paste into. |
| Width / Height | Size of the region to copy, in tile units (max 31 per axis). |
| Commit render | Check to re-render each copied row immediately. Leave unchecked for off-screen regions. |
| Destination relative to camera scroll | Treat Destination X/Y as screen coordinates instead of absolute scene tile coordinates. |

> **Screen positions.** With **relative to camera scroll** ticked, the position is measured from the top left tile currently visible, and the camera's position is added for you. That is the same option the SubmappingEx plugin's background events have. At 16px the result is rounded down to the enclosing block, so a camera position halfway through a block snaps back.

<img width="742" height="149" alt="image" src="https://github.com/user-attachments/assets/6fb9d619-378c-4c27-a402-ebd32647708d" />

---

### Redraw meta tiles

Shown in scripts as *Redraw meta tiles*.

Redraws a rectangle of the screen from the map in memory, including its colours on Game Boy Color.

Nothing in SRAM is modified; this event only pushes what is already there back into VRAM. Use it after changing map data with **Commit render** left unchecked, or after anything has written to the background tilemap in VRAM directly (a raw GBVM script, another plugin, a transition effect) and left cells showing the wrong tile.

| Field | Description |
|-------|-------------|
| X / Y | Top left corner of the rectangle, in **tiles** rather than blocks, at both sizes. |
| Width / Height | Size of the region, also in raw tiles. |
| Position relative to camera scroll | Treat X/Y as screen coordinates instead of absolute scene tile coordinates. |

> **Screen positions.** With **relative to camera scroll** ticked, the position is measured from the top left tile currently visible, and the camera's position is added for you. That is the same option the SubmappingEx plugin's background events have. At 16px the result is rounded down to the enclosing block, so a camera position halfway through a block snaps back.

> **On-screen regions only.** Like the **Commit render** flag, this event maps the coordinate to a VRAM cell by wrapping it (`x & 31`, `y & 31`), so a region that is not currently visible overwrites whichever cells happen to occupy that wrapped position and corrupts the display. Redraw only what is on screen.

Redrawing costs one VRAM write per cell, so keep regions as small as the change that prompted them rather than refreshing the whole screen.

---

### Attach a Script to a Metatile Event

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

Unregisters the script currently attached to the selected metatile event slot, stopping it from firing on subsequent occurrences.

| Field | Description |
|-------|-------------|
| Select Metatile Event | The event slot to clear. |

---

## Engine Fields Reference

These are filled in before an attached metatile script runs, and read with **Engine Field Value**.

### Enter event

| Field | Description |
|-------|-------------|
| **Entered Metatile Id** | Which metatile the player has just stepped onto. |
| **Entered Metatile X position** | Its column, in tiles. |
| **Entered Metatile Y position** | Its row, in tiles. |

### Collision event

| Field | Description |
|-------|-------------|
| **Collided Metatile Id** | Which metatile the player was stopped by. |
| **Collided Metatile X position** | Its column, in tiles. |
| **Collided Metatile Y position** | Its row, in tiles. |
| **Collided Metatile Direction** | Which way the player was moving, using GB Studio's usual direction values. |
| **Collided Metatile Source** | Reserved. It records which check produced the collision. |

---

## Animating Tiles

Animated water, waterfalls, flowers and similar effects are set up differently in a
metatile scene than in a stock GB Studio scene. This section explains why the usual
event does not work, and the techniques that do.

### Why "Replace Tile At Position" does not work here

The stock **Replace Tile At Position** event, and its **Sequence** version that GB Studio
normally uses for animated tiles, works by looking up which tile is displayed at a
coordinate and then changing that tile's artwork.

In a scene using this plugin that lookup gives the wrong answer. Every cell of the map now
holds a metatile number rather than a tile, so the event takes the metatile number for a
tile and changes the artwork of something unrelated.

Picking a different coordinate does not help, because the tile displayed at a position is
not in the scene's map at all. The plugin works it out as it draws, from the metatile
scene. For the same reason, the stock **Get Tile At Position** event returns a metatile
number rather than a tile. Use [Get meta tile at position](#get-meta-tile-at-position)
instead.

### Technique 1: change the tile's artwork

Animating a tile on the Game Boy means replacing what that tile looks like. Every cell on
screen using it changes in the same frame, which is exactly what water, waterfalls and
flowers want. The map is untouched, so no metatile numbers change.

So instead of asking which tile is at a coordinate, name the tile directly.

#### Keeping tile indices stable

Set the same **Common Tileset** on the metatile scene and on every scene using metatiles.
GB Studio then places that tileset's tiles first and in order, so a tile sits in the same
slot in every scene, and that slot is just its position in the common tileset image,
counted left to right then top to bottom from 0.

> **Slots above 127.** The first 128 tiles of a tileset go into slots 0 to 127, and the
> rest are placed in a second block whose position depends on how many are left. Position
> equals slot only for the first 128, so keep animated tiles inside the first 128 entries
> of the common tileset and the question never arises. If in doubt, check the slot in an
> emulator's tile viewer, such as BGB or Emulicious.

#### Option A: the replaceTilesetTiles plugin

[gbs-replaceTilesetTilesPlugin](https://github.com/Mico27/gbs-replaceTilesetTilesPlugin)
puts this in a normal event, which is the simplest route.

1. Put the animation frames in their own tileset, for example four frames of water in a
   row, giving positions 0, 1, 2 and 3.
2. Add a **Replace Tileset Tiles** event and set:
   - **Tileset** to the animation tileset;
   - **Target Tile Index** to the slot of the tile being animated, adding 2048 to write
     into the second tile bank on Game Boy Color;
   - **Source Offset Tile Index** to the frame to show, which can be a variable;
   - **Length** to how many tiles in a row to copy, 1 per frame here, or 4 for a whole
     16 pixel block when the frames are stored as groups of four.
3. Drive it from a loop: a **Set Timer Script**, or an On Init script ending in a loop of
   **Wait 8 frames**, then advance the frame variable, then **Replace Tileset Tiles**.

The same plugin's **Replace Tileset Tiles Ex** event takes the source tileset from
variables, if the animation set itself has to be chosen while the game runs.

#### Option B: a GBVM script

The same thing without that plugin, using a **GBVM Script** event. The tile replacement
instruction takes its target and source as variables rather than plain numbers, so push
them first:

```
VM_PUSH_CONST   12                  ; target VRAM slot  -> .ARG1
VM_PUSH_CONST   0                   ; source tile index -> .ARG0
VM_REPLACE_TILE .ARG1, ___bank_tileset_water_anim, _tileset_water_anim, .ARG0, 4
VM_POP          2
```

| Parameter | Meaning |
|---|---|
| `TARGET_TILE_IDX` | Variable holding the first VRAM slot to overwrite. Bit 11 (`0x0800`) selects VRAM bank 1 on Game Boy Color. |
| `TILEDATA_BANK` / `TILEDATA` | Where the source tileset lives, written as `___bank_<symbol>` and `_<symbol>`, using the tileset's symbol from its asset settings. |
| `START_IDX` | Variable holding the first tile to read inside that tileset. |
| `LEN` | Number of consecutive tiles to copy. An immediate, not a variable. |

To advance frames, point `TARGET_TILE_IDX` / `START_IDX` at real script variables
(`VM_SET_CONST`, or a GB Studio variable) instead of pushed constants and update them
between calls.

### Technique 2: swap the metatile, for one position at a time

Technique 1 animates a tile everywhere it appears at once. When a single position has to
animate on its own, such as one flower or one torch, change the metatile instead of the
artwork. Define one metatile per animation frame in the metatile scene, then cycle them
with [Replace meta tile](#replace-meta-tile) at that coordinate, with **Commit render**
ticked so the change reaches the screen.

This costs one metatile number per frame and only redraws the cells you name, so it suits
a handful of animated positions. For a whole map's worth of water, use Technique 1.

---

## FAQ

**How do I build an overworld bigger than GB Studio allows?**
Use 16 pixel metatiles. A 2 by 2 block costs one number instead of four tiles, so a scene of
roughly 254 by 112 tiles fits where a normal scene could not.

**How do I make a change to the map that sticks?**
Use **Replace meta tile**. The map lives in the cartridge's memory, so a bridge you build or a wall
you break stays that way until the scene is loaded again.

**Can I change part of the map the player cannot see?**
Yes, and that is one of the reasons to use this. Leave **Commit render** unticked for anything off
screen. Ticking it for an off-screen position corrupts the display.

**Do my map changes survive a save and load?**
They survive a load, because saves use different memory banks. They are not part of the save file,
so they do not carry between play sessions. Record what matters in variables and rebuild the map
from them.

**Which size should I choose, 8px or 16px?**
16px for a large overworld, where a block of four tiles is the natural unit. 8px when you need to
change individual tiles, at the cost of a smaller maximum map.

**My animated water stopped working after adding metatiles.**
The stock **Replace Tile At Position** event cannot work here, because the map now holds metatile
numbers rather than tiles. See [Animating Tiles](#animating-tiles) for the two techniques that do
work.

**How do I run a script when the player steps on a particular block?**
Turn on **Enable enter metatile event** for your scene type, then use **Attach a Script to a
Metatile Event** in the scene's init script.

**How do I detect the player pushing against a wall?**
Turn on the collision events for your scene type and attach a script to the direction you care
about, or to all four.

**My build failed saying a tile was not found in the metatile scene.**
Every group of tiles in your scene has to exist as a block in the metatile scene. The error names
the coordinate. Add that block, or repaint the scene using blocks that exist.

**My scene is too big and the build refuses it.**
The map has a size limit that depends on the width rounded up to a power of two. The error gives
the exact numbers. Making the scene slightly narrower often helps far more than making it shorter.

**Can I use save memory for something else?**
Not bank 0. The plugin owns it entirely. Banks 1 to 3 are available, and that is where game saves
go.

**Do I need a cartridge with save memory?**
Yes. The map lives there. If your game also uses save slots, it needs at least 32 KiB.

**Can I generate a level while the game runs?**
Yes. Write metatile numbers into the map with **Replace meta tile**, leaving **Commit render**
unticked while it is off screen, then draw it with **Redraw meta tiles**.

**Does it work with the ContinuousScene or ScreenScroll plugins?**
Yes. Compatible variants ship for both, and a 16 pixel example combined with screen scrolling is
included.

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

Each setting changes what gets compiled. Figures are what you **get back by turning
the setting off**. Rows marked *off by default* show what turning it **on** costs, and
sliders show the cost per step. "none" means that budget does not move.

| Setting | Bank 0 | WRAM | Banked ROM |
|---|---|---|---|
| Size in pixels of one metatile → *16px* | +137 B | none | +747 B |
| Minimum metatile index to start checking for the entered metatile event *(slider 0–255, default 1)* | none | none | none |
| Enter metatile event detection mode → *Origin point* | none | none | -367 B |
| Platformer: Enable enter metatile event *(off by default, so this is the cost of turning it on)* | none | none | +16 B |
| Platformer: Enable collision metatile event *(off by default, so this is the cost of turning it on)* | none | none | none |
| Platformer: Enable down collision metatile event *(off by default, so this is the cost of turning it on)* | none | none | +23 B |
| Platformer: Enable right collision metatile event *(off by default, so this is the cost of turning it on)* | none | none | +29 B |
| Platformer: Enable up collision metatile event *(off by default, so this is the cost of turning it on)* | none | none | +23 B |
| Platformer: Enable left collision metatile event *(off by default, so this is the cost of turning it on)* | none | none | +29 B |
| Point N Click: Enable enter metatile event *(off by default, so this is the cost of turning it on)* | none | none | +18 B |
| Adventure: Enable enter metatile event *(off by default, so this is the cost of turning it on)* | none | none | +12 B |
| Adventure: Enable collision metatile event *(off by default, so this is the cost of turning it on)* | none | none | none |
| Adventure: Enable down collision metatile event *(off by default, so this is the cost of turning it on)* | none | none | -35 B |
| Adventure: Enable right collision metatile event *(off by default, so this is the cost of turning it on)* | none | none | +37 B |
| Adventure: Enable up collision metatile event *(off by default, so this is the cost of turning it on)* | none | none | +149 B |
| Adventure: Enable left collision metatile event *(off by default, so this is the cost of turning it on)* | none | none | +34 B |
| Shmup: Enable enter metatile event *(off by default, so this is the cost of turning it on)* | none | none | +19 B |
| Shmup: Enable collision metatile event *(off by default, so this is the cost of turning it on)* | none | none | none |
| Shmup: Enable down collision metatile event *(off by default, so this is the cost of turning it on)* | none | none | -18 B |
| Shmup: Enable right collision metatile event *(off by default, so this is the cost of turning it on)* | none | none | -24 B |
| Shmup: Enable up collision metatile event *(off by default, so this is the cost of turning it on)* | none | none | +37 B |
| Shmup: Enable left collision metatile event *(off by default, so this is the cost of turning it on)* | none | none | +58 B |
| Top Down: Enable enter metatile event *(off by default, so this is the cost of turning it on)* | none | none | +17 B |
| Top Down: Enable collision metatile event *(off by default, so this is the cost of turning it on)* | none | none | none |
| Top Down: Enable down collision metatile event *(off by default, so this is the cost of turning it on)* | none | none | +23 B |
| Top Down: Enable right collision metatile event *(off by default, so this is the cost of turning it on)* | none | none | +24 B |
| Top Down: Enable up collision metatile event *(off by default, so this is the cost of turning it on)* | none | none | +25 B |
| Top Down: Enable left collision metatile event *(off by default, so this is the cost of turning it on)* | none | none | +24 B |

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

GB Studio 4.3.0-e1. This plugin's engine code was compiled with the toolchain and
flags GB Studio itself uses, and the size of each part of the result was read back and
sorted into the three budgets: the fixed bank 0, work RAM, and switchable ROM banks.

Two caveats. Only this plugin's own engine sources are measured, so a setting that also
changes a shared data structure can move a few more bytes elsewhere. And each setting is
toggled on its own, so a few measure slightly *negative* when enabling their code lets
the compiler drop a fallback path, and a setting that gates other settings shows only
its own contribution.

</details>
<!-- SETTINGCOST:END -->

## Memory Footprint

Measured against the stock GB Studio **4.3.0-e1** engine at default engine settings, report of 2026-08-13. Figures are the difference against a stock project: a file that replaces a stock engine file counts only the change, which is why a plugin can come out negative. Each event you use also compiles a few bytes of script into your project, on top of the fixed cost below.

| Budget | Cost |
|---|---|
| Bank 0 (HOME) | -27 bytes |
| WRAM | +54 bytes |
| Banked ROM | +3,952 bytes |

- **Bank 0:** the plugin *gives back* 27 bytes, because its replacements for stock engine files compile smaller than the originals. See [Bank 0 (HOME) Usage](#bank-0-home-usage).
- **WRAM:** 54 bytes, almost all of it metatile state; the reworked scroll code gives part of it back.
- **Banked ROM:** 3,952 bytes at the default 8 pixel size. 22 of those land in stock files the plugin does not ship, which compile slightly differently once it is installed. Switching to 16 pixel metatiles adds 747 bytes of banked ROM and 137 bytes of bank 0, which turns the bank 0 saving into a cost of 110 bytes.
- **Engine WRAM headroom:** a stock GB Studio 4.3.0 project leaves about **854 bytes** of WRAM free (the engine has 7,776 bytes to work with and uses 6,922 of them). With this plugin installed roughly **800 bytes** remain. Adding more global variables to your project does not change that figure, because script memory is a fixed 3,584 byte block at stock engine settings.
- **SRAM:** the plugin takes the **whole 8 KiB of save memory bank 0** for the map and its collision. Game saves move to banks 1 to 3, so a cartridge with at least 32 KiB of save memory is needed when your game uses save slots.

---

<!-- BANK0:BEGIN -->
## Bank 0 (HOME) Usage

Bank 0 is the 16 KB fixed ROM bank shared by the GB Studio engine core, the
interrupt handlers and the GBDK runtime. Extra banked ROM is cheap to add,
bank 0 is not, so bank 0 is usually the first thing a project runs out of.

| | Bytes |
|---|---|
| Bank 0 used by this plugin | **-27** |
| Bank 0 free with this plugin installed | **1,478** of 16,384 (91% used) |

**This plugin gives bank 0 space back.** Its replacements for stock engine
files compile smaller than the originals, freeing 27 bytes.

| Module | This plugin | Stock engine | Bank 0 cost |
|---|---|---|---|
| `core/collision.c` | 339 | 401 | -62 |
| `core/scroll.c` | 321 | 286 | +35 |

A module that replaces a stock engine file costs only the *difference*, because
the stock version's bank 0 bytes were being spent anyway.

<details><summary>How this was measured</summary>

GB Studio 4.3.0-e1, default engine settings. Each module was compiled with the
toolchain and flags GB Studio itself uses, and the bank 0 size the compiler
recorded was read back. The stock column is the same compile of the engine file
the module replaces.

The "free" figure assumes a stock project with this plugin and nothing else.
Your own number will differ, because other plugins and any engine settings that
change what the core compiles move it too.

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
