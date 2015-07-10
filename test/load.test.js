'use strict';

/* jshint node: true */

var fontnik = require('..');
var test = require('tape');
var fs = require('fs');
var mkdirp = require('mkdirp');
var UPDATE = process.env.UPDATE;

var opensans = fs.readFileSync(__dirname + '/fixtures/fonts/open-sans/OpenSans-Regular.ttf');
var notokufiarabic = fs.readFileSync(__dirname + '/fixtures/fonts/noto-kufi-arabic/NotoKufiArabic-Regular.ttf');

test('load', function(t) {
    t.test('loads: Open Sans Regular', function(t) {
        fontnik.load(opensans, function(err, faces) {
            t.error(err);

            var family = faces[0].family_name;
            var style = faces[0].style_name;
            var face = [family, style].join(' ');
            var dir = __dirname + '/expected/' + face;
            var codepoints = dir + '/codepoints.json';

            t.equal(family, 'Open Sans');
            t.equal(style, 'Regular');

            if (UPDATE) {
                mkdirp(dir);
                fs.writeFileSync(codepoints, JSON.stringify(faces[0].points));
            }

            t.deepEqual(faces[0].points, require(codepoints));

            t.end();
        });
    });

    t.test('loads: Noto Kufi Arabic Regular', function(t) {
        fontnik.load(notokufiarabic, function(err, faces) {
            t.error(err);

            var family = faces[0].family_name;
            var style = faces[0].style_name;
            var face = [family, style].join(' ');
            var dir = __dirname + '/expected/' + face;
            var codepoints = dir + '/codepoints.json';

            t.equal(family, 'Noto Kufi Arabic');
            t.equal(style, 'Regular');

            if (UPDATE) {
                mkdirp(dir);
                fs.writeFileSync(codepoints, JSON.stringify(faces[0].points));
            }

            t.deepEqual(faces[0].points, JSON.parse(fs.readFileSync(codepoints)))

            t.end();
        });
    });

    t.test('invalid font loading', function(t) {
        t.throws(function() {
            fontnik.load(undefined, function(err, faces) {});
        }, /First argument must be a font buffer/);
        t.end();
    });

    t.test('non existent font loading', function(t) {
        var doesnotexistsans = new Buffer('baloney');
        fontnik.load(doesnotexistsans, function(err, faces) {
            t.ok(err.message.indexOf('Font buffer is not an object'));
            t.end();
        });
    });

    t.test('load typeerror callback', function(t) {
        t.throws(function() {
            fontnik.load(opensans);
        }, /Callback must be a function/);
        t.end();
    });
});

