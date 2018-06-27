'use strict';

/* jshint node: true */

var fontnik = require('..');
var test = require('tape');
var fs = require('fs');
var path = require('path');
var zlib = require('zlib');
var zdata = fs.readFileSync(__dirname + '/fixtures/range.0.256.pbf');
var Protobuf = require('pbf');
var Glyphs = require('./format/glyphs');
var UPDATE = process.env.UPDATE;

function nobuffer(key, val) {
    return key !== '_buffer' && key !== 'bitmap' ? val : undefined;
}

function jsonEqual(t, key, json) {
    if (UPDATE) fs.writeFileSync(__dirname + '/expected/' + key + '.json', JSON.stringify(json, null, 2));
    t.deepEqual(json, require('./expected/' + key + '.json'));
}

var expected = JSON.parse(fs.readFileSync(__dirname + '/expected/load.json').toString());
var firasans = fs.readFileSync(path.resolve(__dirname + '/../fonts/firasans-medium/FiraSans-Medium.ttf'));
var opensans = fs.readFileSync(path.resolve(__dirname + '/../fonts/open-sans/OpenSans-Regular.ttf'));
var invalid_no_family = fs.readFileSync(path.resolve(__dirname + '/fixtures/fonts-invalid/1c2c3fc37b2d4c3cb2ef726c6cdaaabd4b7f3eb9.ttf'));
var guardianbold = fs.readFileSync(path.resolve(__dirname + '/../fonts/GuardianTextSansWeb/GuardianTextSansWeb-Bold.ttf'));
var osaka = fs.readFileSync(path.resolve(__dirname + '/../fonts/osaka/Osaka.ttf'));

test('load', function(t) {
    t.test('loads: Fira Sans', function(t) {
        fontnik.load(firasans, function(err, faces) {
            t.error(err);
            t.equal(faces[0].points.length, 789);
            t.equal(faces[0].family_name, 'Fira Sans');
            t.equal(faces[0].style_name, 'Medium');
            t.end();
        });
    });

    t.test('loads: Open Sans', function(t) {
        fontnik.load(opensans, function(err, faces) {
            t.error(err);
            t.equal(faces[0].points.length, 882);
            t.equal(faces[0].family_name, 'Open Sans');
            t.equal(faces[0].style_name, 'Regular');
            t.end();
        });
    });

    t.test('loads: Guardian Bold', function(t) {
        fontnik.load(guardianbold, function(err, faces) {
            t.error(err);
            t.equal(faces[0].points.length, 227);
            t.equal(faces[0].hasOwnProperty('family_name'), true);
            t.equal(faces[0].family_name, '?');
            t.equal(faces[0].hasOwnProperty('style_name'), false);
            t.equal(faces[0].style_name, undefined);
            t.end();
        });
    });

    t.test('loads: Osaka', function(t) {
        fontnik.load(osaka, function(err, faces) {
            t.error(err);
            t.equal(faces[0].family_name, 'Osaka');
            t.equal(faces[0].style_name, 'Regular');
            t.end();
        });
    });

    t.test('invalid arguments', function(t) {
        t.throws(function() {
            fontnik.load();
        }, /First argument must be a font buffer/);

        t.throws(function() {
            fontnik.load({});
        }, /First argument must be a font buffer/);

        t.end();
    });

    t.test('non existent font loading', function(t) {
        var doesnotexistsans = Buffer.from('baloney');
        fontnik.load(doesnotexistsans, function(err, faces) {
            t.ok(err.message.indexOf('Font buffer is not an object'));
            t.end();
        });
    });

    t.test('load typeerror callback', function(t) {
        t.throws(function() {
            fontnik.load(firasans);
        }, /Callback must be a function/);
        t.end();
    });

    t.test('load font with no family name', function(t) {
        fontnik.load(invalid_no_family, function(err, faces) {
            t.ok(err.message.indexOf('font does not have family_name') > -1);
            t.equal(faces,undefined);
            t.end();
        });
    });

});

test('range', function(t) {
    var data;
    zlib.inflate(zdata, function(err, d) {
        if (err) throw err;
        data = d;
    });

    t.test('ranges', function(t) {
        fontnik.range({font: opensans, start: 0, end: 256}, function(err, res) {
            t.error(err);
            t.ok(data);

            var zpath = __dirname + '/fixtures/range.0.256.pbf';

            function compare() {
                zlib.inflate(fs.readFileSync(zpath), function(err, inflated) {
                    t.error(err);
                    t.deepEqual(data, inflated);

                    var vt = new Glyphs(new Protobuf(new Uint8Array(data)));
                    var json = JSON.parse(JSON.stringify(vt, nobuffer));
                    jsonEqual(t, 'range', json);

                    t.end();
                });
            }

            if (UPDATE) {
                zlib.deflate(data, function(err, zdata) {
                    t.error(err);
                    fs.writeFileSync(zpath, zdata);
                    compare();
                });
            } else {
                compare();
            }
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
            t.error(err);
            var vt = new Glyphs(new Protobuf(new Uint8Array(res)));
            t.equal(vt.stacks.hasOwnProperty('Open Sans Regular'), true);
            var codes = Object.keys(vt.stacks['Open Sans Regular'].glyphs);
            t.deepEqual(codes, ['34','35','36','37','38']);
            t.end();
        });
    });

    t.test('invalid arguments', function(t) {
        t.throws(function() {
            fontnik.range();
        }, /First argument must be an object of options/);

        t.throws(function() {
            fontnik.range({font:'not an object'}, function(err, data) {});
        }, /Font buffer is not an object/);

        t.throws(function() {
            fontnik.range({font:{}}, function(err, data) {});
        }, /First argument must be a font buffer/);

        t.end();
    });

    t.test('range filepath does not exist', function(t) {
        var doesnotexistsans = Buffer.from('baloney');
        fontnik.range({font: doesnotexistsans, start: 0, end: 256}, function(err, faces) {
            t.ok(err);
            t.equal(err.message, 'could not open font');
            t.end();
        });
    });

    t.test('range invalid font with no family name', function(t) {
        fontnik.range({font: invalid_no_family, start: 0, end: 256}, function(err, faces) {
            t.ok(err);
            t.equal(err.message, 'font does not have family_name');
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

    t.test('range with undefined style_name', function(t) {
        fontnik.range({font: guardianbold, start: 0, end: 256}, function(err, data) {
            t.error(err);
            var vt = new Glyphs(new Protobuf(new Uint8Array(data)));
            t.equal(vt.stacks.hasOwnProperty('?'), true);
            t.equal(vt.stacks['?'].hasOwnProperty('name'), true);
            t.equal(vt.stacks['?'].name, '?');
            t.end();
        });
    });

    t.test('range with osaka', function(t) {
        fontnik.range({font: osaka, start:0, end: 256}, function(err, data) {
            t.error(err);
            var vt = new Glyphs(new Protobuf(new Uint8Array(data)));
            var glyphs = vt.stacks['Osaka Regular'].glyphs;
            var keys = Object.keys(glyphs);

            var glyph;
            for (var i = 0; i < keys.length; i++) {
                glyph = glyphs[keys[i]];
                t.deepEqual(Object.keys(glyph), ['id', 'width', 'height', 'left', 'top', 'advance']);
                t.equal(glyph.width, 0);
                t.equal(glyph.height, 0);
            }

            t.end();
        });
    });
});
