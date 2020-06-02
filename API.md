## `fontnik`

### `range(options: object, callback: function)`

Get a range of glyphs as a protocol buffer. `options` is an object with options:
* `font: buffer`
* `start: number`
* `end: number`

`font` is the actual font file.

`callback` will be called as `callback(err, res)` where `res` is the protocol buffer result.

### `load(font: buffer, callback: function)`

Read a font's metadata. Returns an object like
``` json
"family_name": "Open Sans",
"style_name": "Regular",
"points": [32,33,34,35…]
```
where `points` is an array of numbers corresponding to unicode points where this font face has coverage.

`callback` will be called as `callback(err, res)` where `res` is an array of font style object metadata.

### `composite(buffers: [buffer], callback: function)`

Combine any number of glyph (SDF) PBFs. Returns a re-encoded PBF with the combined font faces, composited using array order to determine glyph priority.

`callback` will be called as `callback(err, res)` where `res` is the composited protocol buffer result.
