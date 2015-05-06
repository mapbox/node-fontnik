## `fontnik`

### `range(options: object, callback: function)`

Get a range of glyphs as a protocol buffer. `options` is an object with options:
* `font: buffer`
* `start: number`
* `end: number`

`font` is the actual font file.

### `load(font: buffer, callback: function)`

Read a font's metadata. Returns an object like
``` json
"family_name": "Open Sans",
"style_name": "Regular",
"points": [32,33,34,35…]
```
where `points` is an array of numbers corresponding to unicode points where this font face has coverage.
