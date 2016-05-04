'use strict';

/* jshint node: true */

var fontnik = require('..');
var test = require('tap').test;
var fs = require('fs');
var path = require('path');
var Protobuf = require('pbf');
var Font = require('./format/glyphs').Font.read;
var UPDATE = process.env.UPDATE;

var opensans = fs.readFileSync(path.resolve(__dirname + '/../fonts/open-sans/OpenSans-Regular.ttf'));

function noBuffer(key, val) {
    return key !== '_buffer' && key !== 'bitmap' ? val : undefined;
}

test('range', function(t) {
    var start = 0;
    var end = 255;
    var key = [start, end].join('-');

    fontnik.range({font: opensans, start: start, end: end}, function(err, res) {
        t.error(err);
        t.ok(res);

        var pbf = new Protobuf(res);
        var json = JSON.parse(JSON.stringify(new Font(pbf), noBuffer));

        var expected = [__dirname, 'expected', 'range', key].join('/');

        var pbfPath = [expected, '.pbf'].join('');
        var jsonPath = [expected, '.json'].join('');

        if (UPDATE) {
            fs.writeFileSync(pbfPath, pbf.buf);
            fs.writeFileSync(jsonPath, JSON.stringify(json, null, 2));
        }

        t.deepEqual(pbf.buf, new Protobuf(fs.readFileSync(pbfPath)).buf);
        t.deepEqual(json, require(jsonPath));

        t.end();
    });
});

/*
test('shape', function(t) {
    fontnik.shape();
    t.end();
});
*/
