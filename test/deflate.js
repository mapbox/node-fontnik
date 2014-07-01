var fontnik = require('../index.js');
var assert = require('assert');
var zlib = require('zlib');
var fs = require('fs');
var zdata = fs.readFileSync(__dirname + '/fixtures/range.0.256.pbf');

describe('deflate', function() {
    var data;
    var opts;
    before(function(done) {
        zlib.inflate(zdata, function(err, d) {
            assert.ifError(err);
            data = d;
            done();
        });
    });
    beforeEach(function(done) {
        opts = {};
        done();
    });

    it('deflated size', function(done) {
        opts.start = 0;
        opts.end = 256;
        opts.deflate = true;
        fontnik.range(opts, function(err, data) {
            assert.ifError(err);
            assert.equal(data.length, zdata.length)
            done();
        });
    });
});
