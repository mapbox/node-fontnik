var fontserver = require('../index.js');
var path = require('path');
var zlib = require('zlib');
var fs = require('fs');
var zdata = fs.readFileSync(__dirname + '/fixtures/mapbox-streets-v4.13.1306.3163.vector.pbf');
var Protobuf = require('./format/protobuf');
var VectorTile = require('./format/vectortile');
var mapnik = require('tilelive-vector').mapnik;

function nobuffer(key, val) {
    return key !== '_buffer' && key !== 'bitmap' ? val : undefined;
}

mapnik.register_fonts(path.resolve(__dirname + '../fonts'), {recurse: true});

zlib.inflate(zdata, function(err, d) {
    var data = d;

    var remaining = 1000;
    for (var i = 0; i < 1000; i++) (function() {
        var tile = new fontserver.Tile(data);
        tile.shape('Open Sans Regular', function(err) {
            var vt = new VectorTile(new Protobuf(new Uint8Array(tile.serialize())));
            var json = JSON.parse(JSON.stringify(vt, nobuffer));
            if (!--remaining) console.log('done');
        });
    })();
});
