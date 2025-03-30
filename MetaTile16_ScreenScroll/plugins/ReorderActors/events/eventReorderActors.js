const l10n = require("../helpers/l10n").default;

export const id = "REORDER_ACTORS";
export const groups = ["Reorder Actors"];
export const name = "Reorder Actors: Tile Y Sort";

export const fields = [
    {
		label: "Reorder Actors: Tile Y Sort"
    },
  ];

export const compile = (input, helpers) => {
	const { _callNative } = helpers;

    _callNative("reorder_all");
};

module.exports = {
  id,
  name,
  groups,
  fields,
  compile,
  allowedBeforeInitFade: true,
};