## `fontnik`

### `range(options: object, callback: function)`

Get a range of glyphs as a protocol buffer. `options` is an object with options:
* `file: string`
* `start: number`
* `end: number`

`file` is the path to a font file on disk.]

### `load(fontpath: string, callback: function)`

Read a font's metadata. Returns an object like
``` json
"family_name": "Open Sans",
"style_name": "Regular",
"points": [32,33,34,35…]
```
where `points` is an array of numbers corresponding to unicode points where this font face has coverage.
