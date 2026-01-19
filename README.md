# gbs-MetatilePlugin
 Metatile plugin for gbstudio (version 16px and 8px)

Contains 3 version of the plugin that allows metatiles in gbstudio
- First version "Metatile16" uses 16px metatiles and is more general purpose, the example project showcases a large map that will allow tile editing and persistance, dimensions can be edited in the metatile.h file.
- Second version "Metatile16_ScreenScroll" uses 16px metatiles and is a single screen scene that can scroll into another scene a la Link's awakening (basicaly a merge of Metatile16 and this plugin: https://github.com/Mico27/GBS-scrollScenePlugin)
 (Requires latest dev version of gbstudio)

- Third version "Metatile8" uses 8px metatiles and is more general purpose, the example project showcases a large map that will allow tile editing and persistance, dimensions can be edited in the metatile.h file.

How to use:
- Create a tileset that will contains all the unique tiles needed, those will be assigned to the common tileset of both the metatile scene and the main scene.
- Create a metatile scene that will contain all the metatiles (max 256) the metatile scene must be 256 pixel wide (for the 16px version) or 128 pixel wide (for the 8px version)
- In the scene that will use the metatiles, put the "Load meta tiles" event and assign it the metatile scene. 
  When compiling, the tilemap will be reconstructed by selecting matching metatile from its scene and the metatile scene.
  additional options can be checked:
  - "Must match metatile color attributes" will try to match the color attributes (Useful if you have metatiles that uses the same tile but with a different color)
  - "Must match metatile collision" will try to match collision attributes (Useful if you have metatiles that uses the same tile but with different collision)

- the "Get meta tile at position" event allows you to get which metatile ID is at a given position in the current scene
- the "Replace meta tile" event allows you to replace at metatile at a given position with the option to commit the render or not (have this unchecked if you replace a metatile that is offscreen)
- the "Submap metatiles" event allows you to copy a section of metatiles from another scene and paste it at a given position in the current scene with the option to commit the render or not (have this unchecked if offscreen)

Since the tilemap is loaded in memory, any metatile replacement are kept in memory and wont reset even if offscreen. This also allows you to replace metatiles in the scene's init script before it is rendered.
You could even dynamicaly generate the scene this way.

- The main scenes must respect the dimensions described as such: (the width in tiles rounded to the upper power of two) x (the height in tile) must not exceed the maximum map size configured in the settings (width x height) cannot exceed 8k.
- Also the bigger you make the max scene size, the less space will be available for save data. If the space begins to become too small for the default GBS Save function, this plugin will help saving data optimaly with minimal space: https://github.com/Mico27/gbs-ConfigLoadSavePlugin

![image](https://github.com/user-attachments/assets/d5ad2258-30f5-4c55-ae53-806cf7f2e769)

This means if you configure the scene max width and max height as such, this maximum amount is 128x16 (2048). This doesnt necessarily mean you cant have a scene Higher than 16, but as long as  (the width in tiles rounded to the upper power of two) x (the height in tile) doesnt exceed 2048.

Events:
- Load meta tiles. The main event that allows to mark a scene as having metatiles and specifie the scene that contains the set of metatiles. Each tiles in the scene must match the tiles in the metatile scene. If using color only, check the "must match metatile color attributes" and color the scene accordingly. Check "Must match metatile collision" if you have metatiles that have the same tile/color data but have different collision data.

<img width="739" height="130" alt="image" src="https://github.com/user-attachments/assets/aad82d0e-f5a5-44b6-a0ea-b2d042aa8ea1" />

- Get meta tile at position. Store the Id of the metatile specified its position in the scene into a variable.

<img width="743" height="91" alt="image" src="https://github.com/user-attachments/assets/0db265a2-d7a4-4a9b-8e82-ab56b220aa27" />

- Get meta tile collision at position. Store the collision data of the metatile specified its position in the scene into a variable.

<img width="742" height="95" alt="image" src="https://github.com/user-attachments/assets/d4ed93ae-e6e9-4b93-a323-17c940d9db50" />

- Assign meta tiles. Assign a metatile at a position by specifying its ID. Check commit render to update the change on screen, keep unchecked if offscreen.

<img width="731" height="135" alt="image" src="https://github.com/user-attachments/assets/c5c1e68a-d0f8-4e37-b58c-b431a1c1c6d1" />

- Assign meta collision. Change the collision data of a metatile specified by its ID.

<img width="740" height="96" alt="image" src="https://github.com/user-attachments/assets/8976b4f5-7a8f-42e4-9668-efc1d56533b9" />

- Reset meta tiles. Reset the any changed metatile at the specified position. Check commit render to update the change on screen, keep unchecked if offscreen.

<img width="741" height="97" alt="image" src="https://github.com/user-attachments/assets/a33d93e8-d13a-4df1-8b80-2ba926d8d0ff" />

- Copy scene submap to background. Submap a section of metatiles from a separate scene into the active scene.

<img width="742" height="149" alt="image" src="https://github.com/user-attachments/assets/6fb9d619-378c-4c27-a402-ebd32647708d" />

- Attach a script to a metatile event. run a script on one of the following events:
- Metatile Enter. Will run the script whenever the player's collision box enters a new tile. the position and ID of the entered tile is stored in the engine fields: Entered Metatile Id, Entered Metatile X position, Entered Metatile Y position.

<img width="738" height="589" alt="image" src="https://github.com/user-attachments/assets/31169949-4a2d-4846-ac11-b6c887d5b19f" />

You can optimize the event by specifying the Minimum metatile index to start checking for the entered metatile event in the settings.

- Metatile Collision (not implemented yet)

<img width="745" height="213" alt="image" src="https://github.com/user-attachments/assets/2494bae8-3f9d-4efd-b397-f4faa3027ef6" />
 
https://github.com/user-attachments/assets/854163c6-284a-4cfd-9dc4-24ba58504804

![image](https://github.com/user-attachments/assets/7fb07219-327f-4818-ba40-7e9a12484f4f)


https://github.com/user-attachments/assets/26751480-4c02-45ea-af46-df208dc619e9

![image](https://github.com/user-attachments/assets/61145b99-31a3-4ed2-912f-bbd7e786c066)


https://github.com/user-attachments/assets/72537786-be55-4dd1-968d-01b5c69c12fc

![image](https://github.com/user-attachments/assets/f6491b28-919a-4043-999f-effef4ac3023)

![SceneRendering3](https://github.com/user-attachments/assets/570cead9-04eb-4df7-8af4-e04235fbccb2)
