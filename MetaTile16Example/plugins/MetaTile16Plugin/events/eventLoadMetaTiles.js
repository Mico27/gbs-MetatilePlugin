export const id = "EVENT_LOAD_META_TILES";
export const name = "Load meta tiles";
export const groups = ["Meta Tiles"];

export const autoLabel = (fetchArg) => {
  return `Load meta tiles`;
};

export const fields = [
  {
    key: "sceneId",
    label: "Metatile Scene",
    type: "scene",
	width: "100%",
    defaultValue: "LAST_SCENE",
  },
  {
    key: "matchColor",
    label: "Must match metatile color attributes",
    type: "checkbox",
    defaultValue: false,
  },
  {
    key: "matchCollision",
    label: "Must match metatile collision",
    type: "checkbox",
    defaultValue: false,
  },
];

const background_cache = {};

export const compile = (input, helpers) => {
  const { options, _callNative, _stackPushConst, _stackPush, _stackPop, _addComment, _declareLocal, variableSetToScriptValue, writeAsset } = helpers;
  
  const { scenes, scene, engineFields } = options;
  const metatile_scene = scenes.find((s) => s.id === input.sceneId);
  if (!metatile_scene) {
    return;
  }
  
  if (!background_cache[scene.backgroundId]){
    let width = scene.background.width >> 1;  
    width--;
    width |= width >> 1;
    width |= width >> 2;
    width |= width >> 4;
    width |= width >> 8;
    width++;  
    const height = scene.background.height >> 1;  
    if (width * height > 7168){
        throw new Error(`The background's width is: ${(scene.background.width >> 1)} which its upper power of two is: ${width} multiplied by the background height: ${height} totals: ${width * height} which exceeds the limit of 7168. Please reduce the scene size.`);
    }
	const newTilemapData = [];
	const oldTilemapData = scene.background.tilemap.data;
	const oldTilemapAttrData = scene.background.tilemapAttr?.data;
	const oldCollisionData = scene.collisions;
	const metaTilemapData = metatile_scene.background.tilemap.data;
	const metaTilemapAttrData = metatile_scene.background.tilemapAttr?.data;
	const metaCollisionData = metatile_scene.collisions;
	let tile_found = false;
	for (let y = 0; y < scene.background.height >> 1; y++){
		for (let x = 0; x < scene.background.width >> 1; x++){
			for (let y2 = 0; y2 < metatile_scene.background.height >> 1; y2++){
				for (let x2 = 0; x2 < metatile_scene.background.width >> 1; x2++){
					if ((oldTilemapData[((y << 1) * scene.background.width) + (x << 1)] == metaTilemapData[((y2 << 1) * metatile_scene.background.width) + (x2 << 1)] &&
						oldTilemapData[((y << 1) * scene.background.width) + (x << 1) + 1] == metaTilemapData[((y2 << 1) * metatile_scene.background.width) + (x2 << 1) + 1] &&
						oldTilemapData[((y << 1) * scene.background.width) + (x << 1) + 1 + scene.background.width] == metaTilemapData[((y2 << 1) * metatile_scene.background.width) + (x2 << 1) + 1 + metatile_scene.background.width] &&
						oldTilemapData[((y << 1) * scene.background.width) + (x << 1) + scene.background.width] == metaTilemapData[((y2 << 1) * metatile_scene.background.width) + (x2 << 1) + metatile_scene.background.width]) &&
						(!input.matchColor || !oldTilemapAttrData || !metaTilemapAttrData || 
						(oldTilemapAttrData[((y << 1) * scene.background.width) + (x << 1)] == metaTilemapAttrData[((y2 << 1) * metatile_scene.background.width) + (x2 << 1)] &&
						oldTilemapAttrData[((y << 1) * scene.background.width) + (x << 1) + 1] == metaTilemapAttrData[((y2 << 1) * metatile_scene.background.width) + (x2 << 1) + 1] &&
						oldTilemapAttrData[((y << 1) * scene.background.width) + (x << 1) + 1 + scene.background.width] == metaTilemapAttrData[((y2 << 1) * metatile_scene.background.width) + (x2 << 1) + 1 + metatile_scene.background.width] &&
						oldTilemapAttrData[((y << 1) * scene.background.width) + (x << 1) + scene.background.width] == metaTilemapAttrData[((y2 << 1) * metatile_scene.background.width) + (x2 << 1) + metatile_scene.background.width])) &&
						(!input.matchCollision || !oldCollisionData || !metaCollisionData || 
						(oldCollisionData[((y << 1) * scene.background.width) + (x << 1)] == metaCollisionData[((y2 << 1) * metatile_scene.background.width) + (x2 << 1)] &&
						oldCollisionData[((y << 1) * scene.background.width) + (x << 1) + 1] == metaCollisionData[((y2 << 1) * metatile_scene.background.width) + (x2 << 1) + 1] &&
						oldCollisionData[((y << 1) * scene.background.width) + (x << 1) + 1 + scene.background.width] == metaCollisionData[((y2 << 1) * metatile_scene.background.width) + (x2 << 1) + 1 + metatile_scene.background.width] &&
						oldCollisionData[((y << 1) * scene.background.width) + (x << 1) + scene.background.width] == metaCollisionData[((y2 << 1) * metatile_scene.background.width) + (x2 << 1) + metatile_scene.background.width]))){
							newTilemapData[(y * (scene.background.width >> 1)) + x] = (y2 * (metatile_scene.background.width >> 1)) + x2;
							tile_found = true;
							break;
					}
				}
				if (tile_found){
					break;
				}
			}
			if (!tile_found){
				throw new Error(`Cannot find matching metatile for tile at coordinate ${(x << 1)}, ${(y << 1)}`);
			}
			tile_found = false;
		}
	}
	scene.background.tilemap.data = newTilemapData; 
	scene.background.tilemapAttr.data = [0]; 
	scene.collisions = [0];
    background_cache[scene.backgroundId] = true;
  }
  _addComment("Load meta tiles");
  
  _stackPushConst(`_${metatile_scene.symbol}`);
  _stackPushConst(`___bank_${metatile_scene.symbol}`);
  		
  _callNative("vm_load_meta_tiles");
  _stackPop(2);  
  
};
