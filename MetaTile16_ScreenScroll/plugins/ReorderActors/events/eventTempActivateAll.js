const l10n = require("../helpers/l10n").default;

export const id = "TEMP_ACTIVATE_ALL";
export const groups = ["Reorder Actors"];
export const name = "Reorder Actors: Activate All";

export const fields = [
    {
		label: "Reorder Actors: Activate All"
    },
  ];

export const compile = (input, helpers) => {
	const { _callNative } = helpers;

    _callNative("temp_activate_all");
};

module.exports = {
  id,
  name,
  groups,
  fields,
  compile,
  allowedBeforeInitFade: true,
};