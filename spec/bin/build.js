#!/usr/bin/env node

var fs = require('fs');
var ranges = require('../ranges.js');
var osmium = require('osmium');

var data = {
    'cjk': {
        'china': 'china-latest.osm.pbf',
        'taiwan': 'taiwan-latest.osm.pbf',
        'japan': 'japan-latest.osm.pbf'
    },
    'hangul': {
        'north-korea': 'north-korea-latest.osm.pbf',
        'south-korea': 'south-korea-latest.osm.pbf'
    }
};

Object.keys(ranges).forEach(function(range) {
    var sorted = {};

    Object.keys(data[range]).forEach(function(name) {
        build(range, name, sorted);
    });

    fs.writeFileSync(__dirname + '/../expected/' + range + '-osm.json', JSON.stringify(sorted, null, 2));
});

function build(range, name, sorted) {
    console.time(range + '-' + name);
    console.log('Parsing ' + range + '-' + name + '...');

    var path = __dirname + '/../data/' + data[range][name];
    var charToRange = {};

    if (!fs.existsSync(path)) {
        console.warn('Requires ' + name + ' OSM extract from geofabrik.de, extract.bbbike.org or Overpass API');
        process.exit(1);
    }

    readFile(path, function() {
        sorted[name] = [];
        for (var key in charToRange) sorted[name].push(charToRange[key]);
        sorted[name].sort(function(a, b) {
            return -1 * (a.count - b.count);
            // return b.count - a.count;
        });
        var sliced = {};
        sorted[name].forEach(function(a,i) {
            sliced[a.index] = a.script + '-' + i;
        });

        fs.writeFileSync(__dirname + '/../expected/' + name + '-osm.json', JSON.stringify(sliced, null, 2));

        console.timeEnd(range + '-' + name);
    });

    function readFile(path, callback) {
        var file = new osmium.File(path);
        var reader = new osmium.Reader(file);
        var handler = new osmium.Handler();
        handler.on('node', getChars);
        handler.on('way', getChars);
        handler.on('relation', getChars);
        handler.on('done', callback);
        reader.apply(handler);
    }

    function getChars(obj) {
        var name = obj.tags().name;
        if (!name) return;
        for (var i = 0; i < name.length; i++) {
            var charCode = name.charCodeAt(i);
            if (charCode < ranges[range][0]) continue;
            if (charCode > ranges[range][1]) continue;
            charToRange[charCode] = charToRange[charCode] || { index: charCode, count: 0 };
            charToRange[charCode].count++;
        }
    }
}
