'use strict';

/* jshint node: true */

var fontnik = require('..');
var test = require('tape');
var fs = require('fs');
var UPDATE = process.env.UPDATE;

var firasans = fs.readFileSync(__dirname + '/fonts/firasans-medium/FiraSans-Medium.ttf');
var opensans = fs.readFileSync(__dirname + '/fonts/open-sans/OpenSans-Regular.ttf');

test('load', function(t) {
    t.test('loads: fira sans', function(t) {
        fontnik.load(firasans, function(err, faces) {
            t.error(err);

            var family = faces[0].family_name;
            var style = faces[0].style_name;
            var face = [family, style].join(' ');
            var codepoints = __dirname + '/expected/' + face + '/codepoints.json';

            t.equal(family, 'Fira Sans');
            t.equal(style, 'Medium');

            if (UPDATE) fs.writeFileSync(codepoints, JSON.stringify(faces[0].points));
            t.deepEqual(faces[0].points, JSON.parse(fs.readFileSync(codepoints)))

            t.end();
        });
    });

    t.test('loads: open sans', function(t) {
        fontnik.load(opensans, function(err, faces) {
            t.error(err);

            var family = faces[0].family_name;
            var style = faces[0].style_name;
            var face = [family, style].join(' ');
            var codepoints = __dirname + '/expected/' + face + '/codepoints.json';

            t.equal(family, 'Open Sans');
            t.equal(style, 'Regular');

            if (UPDATE) fs.writeFileSync(codepoints, JSON.stringify(faces[0].points));
            t.deepEqual(faces[0].points, require(codepoints));

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
            fontnik.load(firasans);
        }, /Callback must be a function/);
        t.end();
    });
});

