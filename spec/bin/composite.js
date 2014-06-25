#!/usr/bin/env node

var fs = require('fs');

var sorted = JSON.parse(fs.readFileSync(__dirname + '/../expected/sorted-osm.json'));

var merged = Object.keys(sorted).reduce(function(prev, locale) {
    for (var i = 0; i < sorted[locale].length; i++) {
        var glyph = sorted[locale][i];
        prev.hasOwnProperty(glyph.index) ? prev[glyph.index].count += glyph.count : prev[glyph.index] = glyph;
    }
    return prev;
}, {});

var sliced = {};
var composite = freqSort(merged); //.slice(0, 4096);

// Sort by range, then frequency, then Unicode index
composite.sort(function(a, b) {
    if (a.sortIndex === b.sortIndex && a.count === b.count) {
        return a.index - b.index;
    } else if (a.sortIndex === b.sortIndex) {
        return b.count - a.count;
    } else {
        return a.sortIndex - b.sortIndex;
    }
});

composite.forEach(function(a,i) {
    sliced[a.index] = 'cjk-common-' + Math.floor(i/256);
});

fs.writeFileSync(__dirname + '/../expected/cjk-osm.json', JSON.stringify(sliced, null, 2));

function freqSort(glyphs) {
    var frequency = [];
    for (var key in glyphs) frequency.push(glyphs[key]);
    frequency.sort(function(a, b) {
        return b.count - a.count;
    });
    return frequency;
}
