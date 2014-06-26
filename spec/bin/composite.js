#!/usr/bin/env node

var fs = require('fs');
var ranges = require('../ranges.js');

Object.keys(ranges).forEach(function(range) {
    var sorted = JSON.parse(fs.readFileSync(__dirname + '/../expected/' + range + '-osm.json'));

    var merged = Object.keys(sorted).reduce(function(prev, locale) {
        for (var i = 0; i < sorted[locale].length; i++) {
            var glyph = sorted[locale][i];
            prev.hasOwnProperty(glyph.index) ? prev[glyph.index].count += glyph.count : prev[glyph.index] = glyph;
        }
        return prev;
    }, {});

    var composite = freqSort(merged); //.slice(0, 4096);

    // Sort by range, then frequency, then Unicode index
    composite.sort(function(a, b) {
        if (a.count === b.count) {
            return a.index - b.index;
        } else {
            return b.count - a.count;
        }
    });

    var sliced = {};
    var glyphs = {};

    composite.forEach(function(a,i) {
        sliced[a.index] = range + '-common';
        glyphs[a.count + '-' + a.index] = String.fromCharCode(a.index);
    });

    fs.writeFileSync(__dirname + '/../expected/' + range + '-common.json', JSON.stringify(sliced, null, 2));
    fs.writeFileSync(__dirname + '/../expected/' + range + '-glyphs.json', JSON.stringify(glyphs, null, 2));
});


function freqSort(glyphs) {
    var frequency = [];
    for (var key in glyphs) frequency.push(glyphs[key]);
    frequency.sort(function(a, b) {
        return b.count - a.count;
    });
    return frequency;
}
