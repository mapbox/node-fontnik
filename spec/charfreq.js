// Generate common CJK character tables from OpenStreetMap extracts.

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
    'cjk-unified-extension-a': [0x3400, 0x4DBF, 0],
    'cjk-compat-ideograph': [0xF900, 0xFAFF, 1],
    'cjk-symbols': [0x3000, 0x303F, 2],
    'cjk-enclosed': [0x3200, 0x32FF, 3],
    'hangul-compat-jamo': [0x3130, 0x318F, 4],
    'bopomofo': [0x3100, 0x312F, 5],
    'halfwidth-forms': [0xFF00, 0xFFEF, 6],
    'cjk-unified': [0x4E00, 0x9FFF, 7],
    'hiragana': [0x3040, 0x309F, 8],
    'katakana': [0x30A0, 0x30FF, 9],
    'hangul-syllables': [0xAC00, 0xD7AF, 10]
};

var rangeKeys = Object.keys(ranges);

function build() {
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
                    charToRange[charCode] = charToRange[charCode] || { index: charCode, count: 0, script: rangeKeys[j], sortIndex: range[2] };
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
                sliced[a.index] = a.script + '-' + i;
            });
            fs.writeFileSync(__dirname + '/expected/' + name + '-osm.json', JSON.stringify(sliced, null, 2));
            console.timeEnd(name);
        });
    });

    fs.writeFileSync(__dirname + '/expected/sorted-osm.json', JSON.stringify(sorted, null, 2));
}

function composite() {
    var sorted = JSON.parse(fs.readFileSync(__dirname + '/expected/sorted-osm.json'));

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

    fs.writeFileSync(__dirname + '/expected/cjk-osm.json', JSON.stringify(sliced, null, 2));

    function freqSort(glyphs) {
        var frequency = [];
        for (var key in glyphs) frequency.push(glyphs[key]);
        frequency.sort(function(a, b) {
            return b.count - a.count;
        });
        return frequency;
    }
}

module.exports = {
    build: build,
    composite: composite
}
