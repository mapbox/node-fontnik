// Generate common CJK character tables of size 4096.
// - cjk-modern.json uses charfreq-modern.csv (http://lingua.mtsu.edu/chinese-computing/statistics/)
// - cjk-osm.json uses an OpenStreetMap China extract.

csvRanges();
osmRanges();

function csvRanges() {
    var csv = require('fs').readFileSync(__dirname + '/charfreq-modern.csv','utf8').split('\n').slice(0,4096);
    var cjk = {};
    csv.forEach(function(line, i) {
        var char = line.split(',')[1].charCodeAt(0);
        cjk[char] = 'cjk-common-' + Math.floor(i/256);
    });
    require('fs').writeFileSync(__dirname + '/cjk-modern.json', JSON.stringify(cjk, null, 2));
}

function osmRanges() {
    var fs = require('fs');
    if (!fs.existsSync(__dirname + '/data/china-latest.osm.pbf')) {
        console.warn('Requires china-latest.osm.pbf from geofabrik.de');
        process.exit(1);
    }

    var osmium = require('osmium');
    var file = new osmium.File(__dirname + '/data/china-latest.osm.pbf');
    var reader = new osmium.Reader(file);
    var handler = new osmium.Handler();
    handler.on('node', getChars);
    handler.on('way', getChars);
    handler.on('relation', getChars);
    handler.on('done', done);
    var charToRange = {};

    reader.apply(handler);

    function getChars(obj) {
        var name = obj.tags().name;
        if (!name) return;
        for (var i = 0; i < name.length; i++) {
            var charCode = name.charCodeAt(i);
            if (charCode < 0x4E00) continue;
            if (charCode > 0x9FFF) continue;
            type = 'cjk';
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
        var cjk = {};
        sorted.slice(0,4096).forEach(function(a,i) {
            cjk[a.char] = 'cjk-common-' + Math.floor(i/256);
        });
        require('fs').writeFileSync(__dirname + '/cjk-osm.json', JSON.stringify(cjk, null, 2));
    }
}

