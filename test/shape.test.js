'use strict';

/* jshint node: true */

var fontnik = require('..');
var test = require('tap').test;
var fs = require('fs');
var path = require('path');
var Protobuf = require('pbf');
var Glyphs = require('./format/glyphs').Font.read;
var UPDATE = process.env.UPDATE;

var opensans = fs.readFileSync(path.resolve(__dirname + '/../fonts/open-sans/OpenSans-Regular.ttf'));

function nobuffer(key, val) {
    return key !== '_buffer' && key !== 'bitmap' ? val : undefined;
}

test('range', function(t) {
    fontnik.range({font: opensans, start: 0, end: 255}, function(err, res) {
        t.error(err);
        t.ok(res);
        var font = new Glyphs(new Protobuf(new Uint8Array(res)));
        var json = JSON.parse(JSON.stringify(font, nobuffer));
        t.end();
    });
});

test('shape', function(t) {
    fontnik.shape();
    t.end();
});
