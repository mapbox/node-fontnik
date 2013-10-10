
- server loads the vector tile and extracts all labels
- read sdf protocol buffer that contains all the prerendered glyphs for a face
- use regular glyph metrics from all languages that don't need to be shaped

- when requesting a tile for a particular map
    - load vector tile
    - load map style/font stack
    - define base glyph set
    - shape all labels with the desired font stack
    - if shaping results in discrepancies/non-linear char => glyph mapping
        - send shaped glyph metrics
        - send non-base glyphs in vector tile
          => measure how much bigger that makes some vector tiles?

- separate font-tiles (one zoom level above)
    - load vector tiles and extract all labels
     => different fonts? we'd need to know the label font so that we don't have
        to send all glyphs in all weights

- on font change on the client
    => re-request vector tile?
    => separate label shaping endpoint?

- what if we want to show other text that is not encoded in the vector tiles?



=> switch to pango for text layouting
    - multiple lines of text?


- implement font backend that reads directly from sdf files
    - need to extract kerning pairs?

- investigate how to port the node.js module with emscripten to run directly
  in the browser => easy bindings?
  - node::ObjectWrap doesn't derive from v8 objects, but it has many methods
    that take v8 objects as parameters

- store text placement in vector tile?




- curved labels for joined text like arabic?



- create glyph cache on javascript-side
- create sdf glyphs (port from ft project)
- create tile that has data + sdf glyphs


- in main sdf file:
  - ((store character code => glyph mapping)) ==> implement later
  - ((implement later: store common glyphs)
    - how to determine default glyphs in a font?
      => Jōyō kanji list?
      => Frequency analysis?
  - store all glyph metrics
    - DON'T store all glyph bitmaps

- in theme-specific tile:
  - store glyph mapping + positioning per label/fontstack
    - ((ONLY IF it deviates from the plain charcode=>glyph mapping)) => implement later
  - store glyph bitmap




- analyze vector tiles for shaped glyph frequency
  => compile list





