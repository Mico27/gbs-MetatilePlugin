export const id = "EVENT_SET_BOMB_POWERUP";
export const name = "Set bomb powerup";
export const groups = ["Bomberman"];

export const autoLabel = (fetchArg) => {
  return `Set bomb powerup`;
};

export const fields = [
  {
    key: `x`,
    label: "X",
    type: "value",
    width: "50%",
    defaultValue: {
      type: "number",
      value: 0,
    },
  },
  {
    key: `y`,
    label: "Y",
    type: "value",
    width: "50%",
    defaultValue: {
      type: "number",
      value: 0,
    },
  }, 
  {
    key: `powerup_type`,
    label: "Powerup Type",
    type: "value",
    width: "50%",
    defaultValue: {
      type: "number",
      value: 0,
    },
  },
  {
    key: `slot`,
    label: "Slot",
    type: "value",
    width: "50%",
    defaultValue: {
      type: "number",
      value: 0,
    },
  },
];

export const compile = (input, helpers) => {
  
  const { _callNative, _stackPushConst, _stackPush, _stackPop, _addComment, _declareLocal, variableSetToScriptValue, getVariableAlias } = helpers;
  
  const tmp0 = _declareLocal("tmp0", 1, true);
  const tmp1 = _declareLocal("tmp1", 1, true);
  const tmp2 = _declareLocal("tmp2", 1, true);
  const tmp3 = _declareLocal("tmp3", 1, true);
    
  variableSetToScriptValue(tmp0, input.slot);
  variableSetToScriptValue(tmp1, input.powerup_type);
  variableSetToScriptValue(tmp2, input.x);
  variableSetToScriptValue(tmp3, input.y);
      
  _addComment("Set bomb powerup");
  
  _stackPush(tmp3);
  _stackPush(tmp2);
  _stackPush(tmp1);
  _stackPush(tmp0);
  		
  _callNative("vm_set_bomb_powerup");
  _stackPop(4);   
};
