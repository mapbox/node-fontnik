// Generate common CJK character tables of size 4096.
// - cjk-osm.json uses OpenStreetMap extracts.

var fs = require('fs');
var osmium = require('osmium');
var queue = require('queue-async');

var data = {
    'china': 'china-latest.osm.pbf',
    'hong-kong': 'hong-kong.osm.pbf',
    'taiwan': 'taiwan-latest.osm.pbf',
    'japan': 'japan-latest.osm.pbf',
    'korea': 'korea.osm.pbf'
};

var ranges = {
    'cjk-radicals': [0x2E80, 0x2EFF],
    'cjk-symbols': [0x3000, 0x303F],
    'cjk-enclosed': [0x3200, 0x32FF],
    'cjk-compat': [0x3300, 0x33FF],
    'cjk-unified-extension-a': [0x3400, 0x4DBF],
    'cjk-unified': [0x4E00, 0x9FFF],
    'cjk-compat-ideograph': [0xF900, 0xFAFF],
    'cjk-compat-forms': [0xFE30, 0xFE4F],
    'cjk-unified-extension-b': [0x20000, 0x2A6DF],
    'cjk-compat-ideograph-supplement': [0x2F800, 0x2FA1F],
    'hiragana': [0x3040, 0x309F],
    'katakana': [0x30A0, 0x30FF],
    'hangul': [0xAC00, 0xD7AF]
};

var rangeKeys = Object.keys(ranges);

var sorted = {};

Object.keys(data).forEach(function(name) {
    console.time(name);
    console.log('Parsing ' + name + '...');
    var path = __dirname + '/data/' + data[name];

    if (!fs.existsSync(path)) {
        console.warn('Requires ' + name + ' OSM extract from geofabrik.de, extract.bbbike.org or Overpass API');
        process.exit(1);
    }

    var charToRange = {};

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
            for (var j = 0; j < rangeKeys.length; j++) {
                var range = ranges[rangeKeys[j]];
                if (charCode < range[0]) continue;
                if (charCode > range[1]) continue;
                charToRange[charCode] = charToRange[charCode] || { index: charCode, count: 0, script: rangeKeys[j] };
                charToRange[charCode].count++;
                break; //charCode range match
            }
        }
    }

    readFile(path, function done() {
        sorted[name] = [];
        for (var key in charToRange) sorted[name].push(charToRange[key]);
        sorted[name].sort(function(a, b) {
            return -1 * (a.count - b.count);
            // return b.count - a.count;
        });
        var sliced = {};
        sorted[name].forEach(function(a,i) {
            sliced[a.index] = 'common-' + a.script + '-' + Math.floor(i/256);
        });
        fs.writeFileSync(__dirname + '/expected/' + name + '-osm.json', JSON.stringify(sliced, null, 2));
        console.timeEnd(name);
    });
});

function freqSort(glyphs) {
    var frequency = [];
    for (var key in glyphs) frequency.push(glyphs[key]);
    frequency.sort(function(a, b) {
        return b.count - a.count;
    });
    return frequency;
}

function composite() {
    var merged = Object.keys(sorted).reduce(function(prev, locale) {
        for (var i = 0; i < sorted[locale].length; i++) {
            var glyph = sorted[locale][i];
            prev.hasOwnProperty(glyph.index) ? prev[glyph.index].count += glyph.count : prev[glyph.index] = glyph;
        }
        return prev;
    }, {});

    var sliced = {};
    var composite = freqSort(merged).slice(0, 8192);
    composite.forEach(function(a,i) {
        sliced[a.index] = 'common-' + a.script + '-' + Math.floor(i/256);
    });
    fs.writeFileSync(__dirname + '/expected/cjk-osm.json', JSON.stringify(sliced, null, 2));
}

composite();
