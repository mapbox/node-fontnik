// Generate common Hnagul character tables of size 512.
// - hangul-osm.json uses an OpenStreetMap Korea extract.
var osmium = require('osmium');
var queue = require('queue-async');

osmRanges();

function osmRanges() {
    var fs = require('fs');
    if (!fs.existsSync(__dirname + '/data/north-korea.osm') || !fs.existsSync(__dirname + '/data/south-korea.osm')) {
        console.warn('Requires North and South Korea Overpass API extracts');
        process.exit(1);
    }

    var charToRange = {};

    var files = [
        __dirname + '/data/north-korea.osm',
        __dirname + '/data/south-korea.osm'
    ];

    var q = queue(1);

    for (var i = 0; i < files.length; i++) {
        console.log('Starting ' + files[i]);
        q.defer(readFile, files[i]);
    }

    q.awaitAll(done);

    function readFile(filename, callback) {
        var file = new osmium.File(filename);
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
            if (charCode < 0xAC00) continue;
            if (charCode > 0xD7AF) continue;
            type = 'hangul';
            charToRange[charCode] = charToRange[charCode] || { char:charCode, count:0};
            charToRange[charCode].count++;
        }
    }

    function done() {
        var sorted = [];
        for (var key in charToRange) sorted.push(charToRange[key]);
        sorted.sort(function(a, b) {
            return -1 * (a.count - b.count);
        });
        var hangul = {};
        sorted.slice(0,512).forEach(function(a,i) {
            hangul[a.char] = 'hangul-common-' + Math.floor(i/256);
        });
        require('fs').writeFileSync(__dirname + '/hangul-osm.json', JSON.stringify(hangul, null, 2));
    }
}

