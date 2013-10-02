
- server loads the vector tile and extracts all labels

- send a list of all labels to the server

- determine script/language for every label

- mapping language/script => font
  - we can't use unicode ranges because, e.g. for japanese we want to use a different
    font than for chinese, but they have the same unicode range (CJK unification)

- shape every label

- read sdf protocol buffer that contains all the prerendered glyphs

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

- load the sdf file

for now:
unicode-range => font file

[
    { from: 0x0530, to: 0x058F, file: 'test.ttf', index: 0 }
]

for every 

for every font, create a font object
on creation, and store all glyphs as individual buffers + metrics into the object


.shape('test text', fontobject)

returns:
- 
- list of glyphs


=> switch to pango for text layouting
    - multiple lines of text?


- implement font backend that reads directly from sdf files
    - need to extract kerning pairs?

- investigate how to port the node.js module with emscripten to run directly
  in the browser => easy bindings?
  - node::ObjectWrap doesn't derive from v8 objects, but it has many methods
    that take v8 objects as parameters

- store text placement in vector tile?


