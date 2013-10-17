The font server loads vector tiles and map stylesheets. It extracts all labels
required to render the tile and loads all fonts in the font stack. Then, it
layouts the text with the respective writing direction ([Unicode bidi](http://www.unicode.org/reports/tr9/))
and glyph shaping and embeds these shaped glyphs aas well as the positions into
the augmented ("styled") vector tile.

## Installing

Make sure you have `pango` and `protobuf` installed. With homebrew, you can just
type `brew install pango protobuf`. The makefile uses `pkg-config` to find these
libraries and links dynamically to them, so make sure that `pkg-config` can find
them.

To compile this module, type:

```
bin/updateproto.sh
npm install
```

## Running

This server uses a font directory contained in this installation. Add any fonts
you want to use there (or comment out the `FONTCONFIG_PATH` override at the top
of `server.js`).

To start the server, type:

```
node server.js
```
