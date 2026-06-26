'use strict';

// Helper process for overflow.test.js. Rendering a glyph that overflows the
// 32-bit bitmap size used to corrupt the heap and crash the process, so this
// runs in a child process where a crash can be observed via the exit signal.

var fontnik = require('..');
var fs = require('fs');
var path = require('path');
var Protobuf = require('pbf');
var Glyphs = require('./format/glyphs');

var evil = fs.readFileSync(path.resolve(__dirname, 'fixtures/evil.ttf'));

fontnik.range({ font: evil, start: 0, end: 255 }, function(err, res) {
    if (err) {
        process.stdout.write(JSON.stringify({ ok: false, error: err.message }) + '\n');
        return;
    }

    var glyphs = new Glyphs(new Protobuf(new Uint8Array(res)));
    var stackName = Object.keys(glyphs.stacks)[0];
    var glyph = glyphs.stacks[stackName].glyphs[0x41];

    process.stdout.write(JSON.stringify({
        ok: true,
        bytes: res.length,
        glyphPresent: !!glyph,
        glyphWidth: glyph ? glyph.width : null,
        glyphHeight: glyph ? glyph.height : null,
        hasBitmap: glyph ? !!glyph.bitmap : false
    }) + '\n');
});
