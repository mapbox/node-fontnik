var fontserver = require('../index.js');
var assert = require('assert');
var zlib = require('zlib');
var fs = require('fs');
var zdata = fs.readFileSync(__dirname + '/fixtures/mapbox-streets-v4.13.1306.3163.vector.pbf');
var Protobuf = require('./format/protobuf');
var VectorTile = require('./format/vectortile');

function nobuffer(key, val) {
    return key !== '_buffer' && key !== 'bitmap' ? val : undefined;
}

function jsonEqual(key, json) {
    fs.writeFileSync(__dirname + '/expected/'+key+'.json', JSON.stringify(json, null, 2));
    assert.deepEqual(json, require('./expected/'+key+'.json'));
}

describe('convert', function() {
    var data;
    before(function(done) {
        zlib.inflate(zdata, function(err, d) {
            assert.ifError(err);
            data = d;
            done();
        });
    });

    it('serialize', function(done) {
        var tile = new fontserver.Tile(data);
        var vt = new VectorTile(new Protobuf(new Uint8Array(tile.serialize())));
        var json = JSON.parse(JSON.stringify(vt, nobuffer));
        jsonEqual('serialize', json);
        done();
    });

    it('shape', function(done) {
        var tile = new fontserver.Tile(data);
        tile.shape('Open Sans Regular', function(err) {
            assert.ifError(err);
            var vt = new VectorTile(new Protobuf(new Uint8Array(tile.serialize())));
            var json = JSON.parse(JSON.stringify(vt, nobuffer));
            // jsonEqual('shape', json);
            done();
        });
    });

    it('shape (x10)', function(done) {
        this.timeout(10000);
        var remaining = 10;
        for (var i = 0; i < 10; i++) (function() {
            var tile = new fontserver.Tile(data);
            tile.shape('Open Sans Regular', function(err) {
                assert.ifError(err);
                var vt = new VectorTile(new Protobuf(new Uint8Array(tile.serialize())));
                var json = JSON.parse(JSON.stringify(vt, nobuffer));
                jsonEqual('shape', json);
                if (!--remaining) return done();
            });
        })();
    });
});

