const l10n = require("../helpers/l10n").default;

export const id = "TEMP_DEACTIVATE_ALL";
export const groups = ["Reorder Actors"];
export const name = "Reorder Actors: Deactivate All";

export const fields = [
    {
		label: "Reorder Actors: Deactivate All"
    },
  ];

export const compile = (input, helpers) => {
	const { _callNative } = helpers;

    _callNative("temp_deactivate_all");
};

module.exports = {
  id,
  name,
  groups,
  fields,
  compile,
  allowedBeforeInitFade: true,
};