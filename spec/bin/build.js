#!/usr/bin/env node

var fs = require('fs');
var osmium = require('osmium');

var osm = require('../osm.json')
var ranges = require('../ranges.js')

var sorted = {};

Object.keys(osm).forEach(function(name) {
    build(name);
});

fs.writeFileSync(__dirname + '/../expected/sorted-osm.json', JSON.stringify(sorted, null, 2));

function build(name) {
    console.time(name);
    console.log('Parsing ' + name + '...');

    var path = __dirname + '/../data/' + osm[name];
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

        console.timeEnd(name);
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
            for (var j = 0; j < ranges.length; j++) {
                var range = ranges[j];
                if (charCode < range[0]) continue;
                if (charCode > range[1]) continue;
                charToRange[charCode] = charToRange[charCode] || { index: charCode, count: 0, sortIndex: j };
                charToRange[charCode].count++;
                break; //charCode range match
            }
        }
    }
}
