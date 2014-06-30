var fontnik = require('../index.js');
var assert = require('assert');
var zlib = require('zlib');
var fs = require('fs');
var zdata = fs.readFileSync(__dirname + '/fixtures/range.0.256.pbf');
var Protobuf = require('pbf');
var Glyphs = require('./format/glyphs');
var UPDATE = process.env.UPDATE;

function nobuffer(key, val) {
    return key !== '_buffer' && key !== 'bitmap' ? val : undefined;
}

function jsonEqual(key, json) {
    if (UPDATE) fs.writeFileSync(__dirname + '/expected/'+key+'.json', JSON.stringify(json, null, 2));
    assert.deepEqual(json, require('./expected/'+key+'.json'));
}

describe('glyphs', function() {
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

    it('serialize', function(done) {
        // On disk fixture generated with the following code.
        /*
        fontnik.range({
            fontstack:'Open Sans Regular',
            start: 0,
            end: 256
        }, function(err, zdata) {
            if (err) throw err;
            fs.writeFileSync(__dirname + '/fixtures/range.0.256.pbf', zdata);
            done();
        });
        */
        var vt = new Glyphs(new Protobuf(new Uint8Array(fontnik.serialize(data))));
        var json = JSON.parse(JSON.stringify(vt, nobuffer));
        jsonEqual('range', json);
        done();
    });

    it('range', function(done) {
        opts.start = 0;
        opts.end = 256;
        opts.deflate = false;
        fontnik.range(opts, function(err, data) {
            assert.ifError(err);
            var vt = new Glyphs(new Protobuf(new Uint8Array(data)));
            var json = JSON.parse(JSON.stringify(vt, nobuffer));
            jsonEqual('range', json);
            done();
        });
    });

    it('range deflated', function(done) {
        opts.start = 0;
        opts.end = 256;
        opts.deflate = true;
        fontnik.range(opts, function(err, zdata) {
            assert.ifError(err);
            zlib.inflate(zdata, function(err, data) {
                assert.ifError(err);
                var vt = new Glyphs(new Protobuf(new Uint8Array(data)));
                var json = JSON.parse(JSON.stringify(vt, nobuffer));
                jsonEqual('range', json);
                done();
            });
        });
    });

    it('range (default deflated)', function(done) {
        opts.start = 0;
        opts.end = 256;
        fontnik.range(opts, function(err, zdata) {
            assert.ifError(err);
            zlib.inflate(zdata, function(err, data) {
                assert.ifError(err);
                var vt = new Glyphs(new Protobuf(new Uint8Array(data)));
                var json = JSON.parse(JSON.stringify(vt, nobuffer));
                jsonEqual('range', json);
                done();
            });
        });
    });

    // Render a long range of characters which can cause segfaults
    // with V8 arrays ... not sure yet why.
    it('longrange', function(done) {
        opts.start = 0;
        opts.end = 1024;
        fontnik.range(opts, function(err) {
            assert.ifError(err);
            done();
        });
    });

    it('sparse range (chars input)', function(done) {
        opts.name = 'a-and-z';
        opts.range = [('a').charCodeAt(0), ('z').charCodeAt(0)];
        opts.deflate = false;
        fontnik.range(opts, function(err, data) {
            assert.ifError(err);
            var vt = new Glyphs(new Protobuf(new Uint8Array(data)));
            var json = JSON.parse(JSON.stringify(vt, nobuffer));
            jsonEqual('chars', json);
            done();
        });
    });

    it('range typeerror fontstack', function(done) {
        opts.fontstack = 0;
        assert.throws(function() {
            fontnik.range(opts, function() {});
        }, /fontstack must be a string/);
        done();
    });

    it('range typeerror range', function(done) {
        opts.name = 0;
        opts.range = fontnik.getRange(0, 256);
        assert.throws(function() {
            fontnik.range(opts, function() {});
        }, /range must be a string/);
        done();
    });

    it('range typeerror chars', function(done) {
        opts.name = '0-256';
        opts.range = 'foo';
        assert.throws(function() {
            fontnik.range(opts, function() {});
        }, /chars must be an array/);
        done();
    });

    it('range typeerror callback', function(done) {
        opts.start = 0;
        opts.end = 256;
        assert.throws(function() {
            fontnik.range(opts, '');
        }, /callback must be a function/);
        done();
    });

    it('range for fontstack with 0 matching fonts', function(done) {
        opts.fontstack = 'doesnotexist';
        opts.start = 0;
        opts.end = 256;
        fontnik.range(opts, function(err) {
            assert.ok(err);
            assert.equal('Error: Failed to find face doesnotexist', err.toString());
            done();
        });
    });

    it('range for fontstack with 1 bad font', function(done) {
        opts.fontstack = 'Open Sans Regular, doesnotexist';
        opts.start = 0;
        opts.end = 256;
        fontnik.range(opts, function(err) {
            assert.ok(err);
            assert.equal('Error: Failed to find face doesnotexist', err.toString());
            done();
        });
    });

    it('getRange', function(done) {
        var json = JSON.parse(JSON.stringify(fontnik.getRange(0, 256)));
        jsonEqual('getRange', json);
        done();
    });

    // Should error because start is < 0
    it('getRange error start < 0', function() {
        assert.throws(function() {
            fontnik.getRange(-128, 256);
        }, 'Error: start must be a number from 0-65533');
    });

    // Should error because end < start
    it('getRange error end < start', function() {
        assert.throws(function() {
            fontnik.getRange(256, 0);
        }, 'Error: start must be less than or equal to end');
    });

    // Should error because end > 65533
    it('getRange error end > 65533', function() {
        assert.throws(function() {
            fontnik.getRange(0, 65534);
        }, 'Error: end must be a number from 0-65533');
    });
});

