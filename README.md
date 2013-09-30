The font server loads vector tiles and map stylesheets. It extracts all labels
required to render the tile and loads all fonts mapped to the various Unicode
ranges. Then, it tries to recognize the script and writing direction and shapes
the text with harfbuzz.

```js
var blob = fs.readFileSync('fonts/ArialUnicode.ttf');
var font = new fontserver.Font(blob);

console.warn(fontserver.shape("لسعودية كسول الزنجبيل القط", font));
```
