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

IMPORTANT: if you are planning to use the Platformer Plus Plugin with this plugin, use this modified version here: https://github.com/Mico27/GBS_PlatformerPlus

- The main scenes must respect the dimensions described as such: (the width in tiles rounded to the upper power of two) x (the height in tile) must not exceed the maximum map size configured in the settings (width x height)

![image](https://github.com/user-attachments/assets/d5ad2258-30f5-4c55-ae53-806cf7f2e769)


https://github.com/user-attachments/assets/854163c6-284a-4cfd-9dc4-24ba58504804

![image](https://github.com/user-attachments/assets/7fb07219-327f-4818-ba40-7e9a12484f4f)


https://github.com/user-attachments/assets/26751480-4c02-45ea-af46-df208dc619e9

![image](https://github.com/user-attachments/assets/61145b99-31a3-4ed2-912f-bbd7e786c066)


https://github.com/user-attachments/assets/72537786-be55-4dd1-968d-01b5c69c12fc

![image](https://github.com/user-attachments/assets/f6491b28-919a-4043-999f-effef4ac3023)

![SceneRendering3](https://github.com/user-attachments/assets/570cead9-04eb-4df7-8af4-e04235fbccb2)
