var Protobuf = require('pbf');
module.exports = Shaping;

function Shaping() {
    Protobuf.call(this);
}

Shaping.prototype = Object.create(Protobuf.prototype);

Shaping.prototype.writeFaces = function(tag, face) {
    var pbf = new Shaping();
    pbf.writeTaggedString(1, face.family);
    pbf.writeTaggedString(2, face.style);
    for (var id in face.glyphs) {
        pbf.writeGlyph(5, face.glyphs[id]);
    }
    this.writeMessage(tag, pbf);
};

Shaping.prototype.writeGlyph = function(tag, glyph) {
    var pbf = new Protobuf();
    pbf.writeTaggedVarint(1, glyph.id);
    if (glyph.bitmap) pbf.writeTaggedBuffer(2, glyph.bitmap);
    pbf.writeTaggedVarint(3, glyph.width);
    pbf.writeTaggedVarint(4, glyph.height);
    pbf.writeTaggedVarint(5, glyph.left);
    pbf.writeTaggedVarint(6, glyph.top);
    pbf.writeTaggedVarint(7, glyph.advance);
    this.writeMessage(tag, pbf);
};

Shaping.prototype.writeLabels = function(tag, label) {
    var pbf = new Protobuf();
    pbf.writeTaggedString(1, label.text);
    pbf.writeTaggedVarint(2, label.stack);
    pbf.writePackedVarints(3, label.faces);
    pbf.writePackedVarints(4, label.glyphs);
    pbf.writePackedVarints(5, label.x);
    pbf.writePackedVarints(6, label.y);
    this.writeMessage(tag, pbf);
};
