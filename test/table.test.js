'use strict';

/* jshint node: true */

var fontnik = require('..');
var test = require('tape');
var fs = require('fs');
var UPDATE = process.env.UPDATE;

var opensans = fs.readFileSync(__dirname + '/fonts/open-sans/OpenSans-Regular.ttf');

test('table', function(t) {
    t.test('tables', function(t) {
        fontnik.table(opensans, 'GPOS', function(err, res) {
            t.error(err);
            if (UPDATE) fs.writeFileSync(__dirname + '/expected/Open Sans Regular/gpos.sfnt', res);
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
            fontnik.table(opensans, undefined, function() {});
        }, /Second argument must be a string table name/);
        t.end();
    });

    t.test('TypeError string of non-zero size', function(t) {
        t.throws(function() {
            fontnik.table(opensans, '', function() {});
        }, /Second argument must be a string of non-zero size/);
        t.end();
    });

    t.test('TypeError callback', function(t) {
        t.throws(function() {
            fontnik.table(opensans, 'GPOS');
        }, /Callback must be a function/);
        t.throws(function() {
            fontnik.table(opensans, 'GPOS', undefined);
        }, /Callback must be a function/);
        t.end();
    });
});
