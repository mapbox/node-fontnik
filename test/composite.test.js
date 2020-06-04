'use strict';

const fontnik = require('../');
const tape = require('tape');
const fs = require('fs');
const path = require('path');

const protobuf = require('protocol-buffers');
const messages = protobuf(fs.readFileSync(path.join(__dirname, '../proto/glyphs.proto')));
const glyphs =  messages.glyphs;

var openSans512 = fs.readFileSync(__dirname + '/fixtures/opensans.512.767.pbf'),
    arialUnicode512 = fs.readFileSync(__dirname + '/fixtures/arialunicode.512.767.pbf'),
    league512 = fs.readFileSync(__dirname + '/fixtures/league.512.767.pbf'),
    composite512 = fs.readFileSync(__dirname + '/fixtures/opensans.arialunicode.512.767.pbf'),
    triple512 = fs.readFileSync(__dirname + '/fixtures/league.opensans.arialunicode.512.767.pbf');

tape('compositing two pbfs', function(t) {
    fontnik.composite([openSans512, arialUnicode512], (err, data) => {
        var composite = glyphs.decode(data);
        var expected = glyphs.decode(composite512);

        t.ok(composite.stacks, 'has stacks');
        t.equal(composite.stacks.length, 1, 'has one stack');

        var stack = composite.stacks[0];

        t.ok(stack.name, 'is a named stack');
        t.ok(stack.range, 'has a glyph range');
        t.deepEqual(composite, expected, 'equals a server-composited stack');

        composite = glyphs.encode(composite);
        expected = glyphs.encode(expected);

        t.deepEqual(composite, expected, 're-encodes nicely');

        fontnik.composite([league512, composite], (err, data2) => {
          var recomposite = glyphs.decode(data2),
              reexpect = glyphs.decode(triple512);
          
          t.deepEqual(recomposite, reexpect, 'can add on a third for good measure');

          t.end();
        });
   
    });
});
