var fontserver = require('../index.js');
var assert = require('assert');
var zlib = require('zlib');
var zdata = require('fs').readFileSync(__dirname + '/fixtures/mapbox-streets-v4.13.2412.3078.vector.pbf');
var data;

describe('convert inflate', function() {
    it('inflate', function(done) {
        zlib.inflate(zdata, function(err, d) {
            assert.ifError(err);
            done();
        });
    });
});

describe('convert simplify', function() {
    var data;
    before(function(done) {
        zlib.inflate(zdata, function(err, d) {
            assert.ifError(err);
            data = d;
            done();
        });
    });
    it('simplify', function(done) {
        var tile = new fontserver.Tile(data);
        tile.simplify(function(err) {
            assert.ifError(err);
            done();
        });
    });
});

describe('convert shape', function() {
    var tile;
    before(function(done) {
        zlib.inflate(zdata, function(err, d) {
            assert.ifError(err);
            tile = new fontserver.Tile(d);
            tile.simplify(function(err) {
                assert.ifError(err);
                done();
            });
        });
    });
    it('shape', function(done) {
        tile.shape('Open Sans, Jomolhari, Siyam Rupali, Alef, Arial Unicode MS', function(err) {
            assert.ifError(err);
            done();
        });
    });
});

describe('convert serialize', function() {
    var tile;
    before(function(done) {
        zlib.inflate(zdata, function(err, d) {
            assert.ifError(err);
            tile = new fontserver.Tile(d);
            tile.simplify(function(err) {
                assert.ifError(err);
                done();
            });
        });
    });
    it('serialize', function(done) {
        assert.equal(tile.serialize().length, 84926);
        done();
    });
});

