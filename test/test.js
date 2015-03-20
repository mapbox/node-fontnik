var fontnik = require('../index.js');
var assert = require('assert');
var fs = require('fs');
var zlib = require('zlib');
var path = require('path');
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

var expected = JSON.parse(fs.readFileSync(__dirname + '/expected/load.json').toString());
var firasans = fs.readFileSync(path.resolve(__dirname + '/../fonts/firasans-medium/FiraSans-Medium.ttf'));
var opensans = fs.readFileSync(path.resolve(__dirname + '/../fonts/open-sans/OpenSans-Regular.ttf'));
describe('load', function() {
    it('loads: fira sans', function(done) {
        fontnik.load(firasans, function(err, faces) {
            assert.ifError(err);
            assert.deepEqual(faces,expected);
            done();
        });
    });

    it('loads: open sans', function(done) {
        fontnik.load(opensans, function(err, faces) {
            assert.ifError(err);
            assert.equal(faces[0].points.length, 882);
            assert.equal(faces[0].family_name, 'Open Sans');
            assert.equal(faces[0].style_name, 'Regular');
            done();
        });
    });

    it('invalid font loading', function(done) {
        var baloneysans;
        assert.throws(function() {
            fontnik.load(baloneysans, function(err, faces) {});
        }, /First argument must be a font buffer/);
        done();
    });

    it('non existent font loading', function(done) {
        var doesnotexistsans = new Buffer('baloney');
        fontnik.load(doesnotexistsans, function(err, faces) {
            assert.ok(err.message.indexOf('Font buffer is not an object'));
            done();
        });
    });

    it('load typeerror callback', function(done) {
        assert.throws(function() {
            fontnik.load(firasans);
        }, /Callback must be a function/);
        done();
    });

});

describe('range', function() {
        var data;
        before(function(done) {
            zlib.inflate(zdata, function(err, d) {
                assert.ifError(err);
                data = d;
                done();
            });
        });

    it('ranges', function(done) {
        this.timeout(10000);
        fontnik.range({font: opensans, start: 0, end: 256}, function(err, res) {
            assert.ifError(err);
            assert.ok(res);
            assert.deepEqual(res, data);
            var vt = new Glyphs(new Protobuf(new Uint8Array(res)));
            var json = JSON.parse(JSON.stringify(vt, nobuffer));
            jsonEqual('range', json);
            done();
        });
    });

    it('longrange', function(done) {
        this.timeout(10000);
        fontnik.range({font: opensans, start: 0, end: 1024}, function(err, data) {
            assert.ifError(err);
            assert.ok(data);
            done();
        });
    });


    it('range typeerror options', function(done) {
        assert.throws(function() {
            fontnik.range(opensans, function(err, data) {});
        }, /Font buffer is not an object/);
        done();
    });

    it('range filepath does not exist', function(done) {
        var doesnotexistsans = new Buffer('baloney');
        fontnik.range({font: doesnotexistsans, start: 0, end: 256}, function(err, faces) {
            assert.ok(err.message.indexOf('Font buffer is not an object'));
            done();
        });
    });

    it('range typeerror start', function(done) {
        assert.throws(function() {
            fontnik.range({font: opensans, start: 'x', end: 256}, function(err, data) {});
        }, /option `start` must be a number from 0-65535/);
        assert.throws(function() {
            fontnik.range({font: opensans, start: -3, end: 256}, function(err, data) {});
        }, /option `start` must be a number from 0-65535/);
        done();
    });

    it('range typeerror end', function(done) {
        assert.throws(function() {
            fontnik.range({font: opensans, start: 0, end: 'y'}, function(err, data) {});
        }, /option `end` must be a number from 0-65535/);
        assert.throws(function() {
            fontnik.range({font: opensans, start: 0, end: 10000000}, function(err, data) {});
        }, /option `end` must be a number from 0-65535/);
        done();
    });

    it('range typeerror lt', function(done) {
        assert.throws(function() {
            fontnik.range({font: opensans, start: 256, end: 0}, function(err, data) {});
        }, /`start` must be less than or equal to `end`/);
        done();
    });

    it('range typeerror callback', function(done) {
        assert.throws(function() {
            fontnik.range({font: opensans, start: 0, end: 256}, '');
        }, /Callback must be a function/);
        assert.throws(function() {
            fontnik.range({font: opensans, start: 0, end: 256});
        }, /Callback must be a function/);
        done();
    });
});
