[![Build Status](https://magnum.travis-ci.com/mapbox/fontserver.png?token=ctvz1otCksqcmNzRzzxa&branch=master)](https://magnum.travis-ci.com/mapbox/fontserver)

fontserver
----------
Is no longer a server, but a library that augments a given mapnik vector tile
with the glyphs needed to render the tile.

Fontserver requires a vector tile buffer to be passed to it and a "font stack"
string of one or more fonts. It extracts all labels required to render the tile
and loads all fonts in the font stack. Then, it layouts the text with the
respective writing direction ([Unicode bidi](http://www.unicode.org/reports/tr9/))
and glyph shaping and embeds these shaped glyphs aas well as the positions into
the augmented ("styled") vector tile.


## Installing

Make sure you have `pango` and `protobuf` installed. With homebrew, you can just
type `brew install pango protobuf`. The makefile uses `pkg-config` to find these
libraries and links dynamically to them, so make sure that `pkg-config` can find
them.

```
npm install
```

## Usage

This is included in the following projects:

- `llmr`
- `tilestream-pro` (`gl` branch)
