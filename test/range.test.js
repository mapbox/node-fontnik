'use strict';

/* jshint node: true */

var fontnik = require('..');
var test = require('tape');
var fs = require('fs');
var zlib = require('zlib');
var zdata = fs.readFileSync(__dirname + '/fixtures/range.0.256.pbf');
var Protobuf = require('pbf');
var Glyphs = require('./format/glyphs');
var UPDATE = process.env.UPDATE;

var opensans = fs.readFileSync(__dirname + '/fonts/open-sans/OpenSans-Regular.ttf');

test('range', function(t) {
    var data;
    zlib.inflate(zdata, function(err, d) {
        if (err) throw err;
        data = d;
    });

    t.test('ranges', function(t) {
        fontnik.range({font: opensans, start: 0, end: 256}, function(err, res) {
            t.error(err);
            t.ok(res);
            t.deepEqual(res, data);

            var vt = new Glyphs(new Protobuf(new Uint8Array(res)));
            var json = JSON.parse(JSON.stringify(vt, function(key, val) {
                return key !== '_buffer' && key !== 'bitmap' ? val : undefined;
            }));

            var range = __dirname + '/expected/Open Sans Regular/range.json';

            if (UPDATE) fs.writeFileSync(range, JSON.stringify(json, null, 2));
            t.deepEqual(json, require(range));

            t.end();
        });
    });

    t.test('longrange', function(t) {
        fontnik.range({font: opensans, start: 0, end: 1024}, function(err, data) {
            t.error(err);
            t.ok(data);
            t.end();
        });
    });

    t.test('shortrange', function(t) {
        fontnik.range({font: opensans, start: 34, end: 38}, function(err, res) {
            var vt = new Glyphs(new Protobuf(new Uint8Array(res)));
            var codes = Object.keys(vt.stacks['Open Sans Regular'].glyphs);
            t.deepEqual(codes, ['34','35','36','37','38']);
            t.end();
        });
    });

    t.test('range typeerror options', function(t) {
        t.throws(function() {
            fontnik.range(opensans, function(err, data) {});
        }, /Font buffer is not an object/);
        t.end();
    });

    t.test('range filepath does not exist', function(t) {
        var doesnotexistsans = new Buffer('baloney');
        fontnik.range({font: doesnotexistsans, start: 0, end: 256}, function(err, faces) {
            t.ok(err.message.indexOf('Font buffer is not an object'));
            t.end();
        });
    });

    t.test('range typeerror start', function(t) {
        t.throws(function() {
            fontnik.range({font: opensans, start: 'x', end: 256}, function(err, data) {});
        }, /option `start` must be a number from 0-65535/);
        t.throws(function() {
            fontnik.range({font: opensans, start: -3, end: 256}, function(err, data) {});
        }, /option `start` must be a number from 0-65535/);
        t.end();
    });

    t.test('range typeerror end', function(t) {
        t.throws(function() {
            fontnik.range({font: opensans, start: 0, end: 'y'}, function(err, data) {});
        }, /option `end` must be a number from 0-65535/);
        t.throws(function() {
            fontnik.range({font: opensans, start: 0, end: 10000000}, function(err, data) {});
        }, /option `end` must be a number from 0-65535/);
        t.end();
    });

    t.test('range typeerror lt', function(t) {
        t.throws(function() {
            fontnik.range({font: opensans, start: 256, end: 0}, function(err, data) {});
        }, /`start` must be less than or equal to `end`/);
        t.end();
    });

    t.test('range typeerror callback', function(t) {
        t.throws(function() {
            fontnik.range({font: opensans, start: 0, end: 256}, '');
        }, /Callback must be a function/);
        t.throws(function() {
            fontnik.range({font: opensans, start: 0, end: 256});
        }, /Callback must be a function/);
        t.end();
    });
});
