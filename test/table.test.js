'use strict';

/* jshint node: true */

var fontnik = require('..');
var test = require('tape');
var fs = require('fs');
var mkdirp = require('mkdirp');
var UPDATE = process.env.UPDATE;

var notokufiarabic = fs.readFileSync(__dirname + '/fixtures/fonts/noto-kufi-arabic/NotoKufiArabic-Regular.ttf');

test('table', function(t) {
    t.test('tables', function(t) {
        fontnik.table(notokufiarabic, 'GPOS', function(err, res) {
            t.error(err);

            var dir = __dirname + '/expected/Noto Kufi Arabic Regular';

            if (UPDATE) {
                mkdirp(dir); 
                fs.writeFileSync(dir + '/gpos.sfnt', res);
            }

            t.ok(res);
            t.end();
        });
    });

    t.test('TypeError font buffer', function(t) {
        t.throws(function() {
            fontnik.table();
        }, /First argument must be a font buffer/);
        t.throws(function() {
            fontnik.table({});
        }, /First argument must be a font buffer/);
        t.end();
    });

    t.test('TypeError string table name', function(t) {
        t.throws(function() {
            fontnik.table(notokufiarabic, undefined, function() {});
        }, /Second argument must be a string table name/);
        t.end();
    });

    t.test('TypeError string of non-zero size', function(t) {
        t.throws(function() {
            fontnik.table(notokufiarabic, '', function() {});
        }, /Second argument must be a string of non-zero size/);
        t.end();
    });

    t.test('TypeError callback', function(t) {
        t.throws(function() {
            fontnik.table(notokufiarabic, 'GPOS');
        }, /Callback must be a function/);
        t.throws(function() {
            fontnik.table(notokufiarabic, 'GPOS', undefined);
        }, /Callback must be a function/);
        t.end();
    });
});
