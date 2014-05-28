var fontserver = require('../index.js');
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
    before(function(done) {
        zlib.inflate(zdata, function(err, d) {
            assert.ifError(err);
            data = d;
            done();
        });
    });

    it('serialize', function(done) {
        // On disk fixture generated with the following code.
        /*
        fontserver.range({
            fontstack:'Open Sans Regular, Siyam Rupali Regular',
            start: 0,
            end: 256
        }, function(err, zdata) {
            if (err) throw err;
            fs.writeFileSync(__dirname + '/fixtures/range.0.256.pbf', zdata);
            done();
        });
        */
        var glyphs = new fontserver.Glyphs(data);
        var vt = new Glyphs(new Protobuf(new Uint8Array(glyphs.serialize())));
        var json = JSON.parse(JSON.stringify(vt, nobuffer));
        jsonEqual('serialize', json);
        done();
    });

    it('range', function(done) {
        var glyphs = new fontserver.Glyphs();
        glyphs.range('Open Sans Regular, Siyam Rupali Regular', 0, 256, function(err) {
            assert.ifError(err);
            var vt = new Glyphs(new Protobuf(new Uint8Array(glyphs.serialize())));
            var json = JSON.parse(JSON.stringify(vt, nobuffer));
            jsonEqual('range', json);
            done();
        });
    });

    it('range typeerror fontstack', function(done) {
        var glyphs = new fontserver.Glyphs();
        assert.throws(function() {
            glyphs.range(0, 0, 256, function() {});
        }, /fontstack must be a string/);
        done();
    });

    it('range typeerror start', function(done) {
        var glyphs = new fontserver.Glyphs();
        assert.throws(function() {
            glyphs.range('Open Sans Regular', 'foo', 256, function() {});
        }, /start must be a number/);
        done();
    });

    it('range typeerror end', function(done) {
        var glyphs = new fontserver.Glyphs();
        assert.throws(function() {
            glyphs.range('Open Sans Regular', 0, 'foo', function() {});
        }, /end must be a number/);
        done();
    });

    it('range typeerror callback', function(done) {
        var glyphs = new fontserver.Glyphs();
        assert.throws(function() {
            glyphs.range('Open Sans Regular', 0, 256, '');
        }, /callback must be a function/);
        done();
    });

    it('range for fontstack with 0 matching fonts', function(done) {
        var glyphs = new fontserver.Glyphs();
        glyphs.range('doesnotexist', 0, 256, function(err) {
            assert.ok(err);
            assert.equal('Error: Failed to find face doesnotexist', err.toString());
            done();
        });
    });

    it('range for fontstack with 1 bad font', function(done) {
        var glyphs = new fontserver.Glyphs();
        glyphs.range('Open Sans Regular, doesnotexist', 0, 256, function(err) {
            assert.ok(err);
            assert.equal('Error: Failed to find face doesnotexist', err.toString());
            done();
        });
    });

    // Should error because start is < 0
    it('range error start < 0', function(done) {
        var glyphs = new fontserver.Glyphs();
        glyphs.range('Open Sans Regular', -128, 256, function(err) {
            assert.ok(err);
            assert.equal('Error: start must be a number from 0-65533', err.toString());
            done();
        });
    });

    // Should error because end < start
    it('range error end < start', function(done) {
        var glyphs = new fontserver.Glyphs();
        glyphs.range('Open Sans Regular', 256, 0, function(err) {
            assert.ok(err);
            assert.equal('Error: start must be less than or equal to end', err.toString());
            done();
        });
    });

    // Should error because end > 65533
    it('range error end > 65533', function(done) {
        var glyphs = new fontserver.Glyphs();
        glyphs.range('Open Sans Regular', 0, 65534, function(err) {
            assert.ok(err);
            assert.equal('Error: end must be a number from 0-65533', err.toString());
            done();
        });
    });
});

