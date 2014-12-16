## `fontnik`

### `range(options: object, callback: function)`

`options` is an object with options:

* `start: number`
* `end: number`
* `fontstack: ?string`

Get a range of glyphs as a gzipped protocol buffer.

### `conf(options: object)`

`options` is an object with options:

* `fonts: Array<string>`

Configure node-fontnik.

### `getRange(start: number, end: number): Array<number>`

Generate an array of the numbers from `start` to `end`. Numbers must be
in order and both less than 65533.

### `faces(): Array<string>`

Return an array of supported font faces, as strings.

## `new fontnik.Glyphs()`

Create a new Glyphs object.

### `glyphs.codepoints(face: string): Array<number>`

Get an array of numbers corresponding to unicode points where this font face
has coverage.

### `glyphs.range(face: string, range: string, chars: Array<number>, callback: function)`

### `glyphs.serialize(): Buffer`
