var fontnik = require('../index.js');
var assert = require('assert');
var zlib = require('zlib');
var fs = require('fs');
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

fontnik.register_fonts(path.resolve(__dirname + '/../fonts/'), { recurse: true });

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
        if (UPDATE) {
            fontnik.range({
                fontstack:'Open Sans Regular',
                start: 0,
                end: 256
            }, function(err, zdata) {
                if (err) throw err;
                fs.writeFileSync(__dirname + '/fixtures/range.0.256.pbf', zdata);
                done();
            });
        }
        var glyphs = new fontnik.Glyphs(data);
        var vt = new Glyphs(new Protobuf(new Uint8Array(glyphs.serialize())));
        var json = JSON.parse(JSON.stringify(vt, nobuffer));
        jsonEqual('range', json);
        done();
    });

    it('range', function(done) {
        var glyphs = new fontnik.Glyphs();
        glyphs.range('Open Sans Regular', '0-256', fontnik.getRange(0, 256), function(err) {
            assert.ifError(err);
            var vt = new Glyphs(new Protobuf(new Uint8Array(glyphs.serialize())));
            var json = JSON.parse(JSON.stringify(vt, nobuffer));
            jsonEqual('range', json);
            done();
        });
    });

    // Render a long range of characters which can cause segfaults
    // with V8 arrays ... not sure yet why.
    it('longrange', function(done) {
        var glyphs = new fontnik.Glyphs();
        glyphs.range('Open Sans Regular', '0-1024', fontnik.getRange(0, 1024), function(err) {
            assert.ifError(err);
            done();
        });
    });

    it('range (chars input)', function(done) {
        var glyphs = new fontnik.Glyphs();
        glyphs.range('Open Sans Regular', 'a-and-z', [('a').charCodeAt(0), ('z').charCodeAt(0)], function(err) {
            assert.ifError(err);
            var vt = new Glyphs(new Protobuf(new Uint8Array(glyphs.serialize())));
            var json = JSON.parse(JSON.stringify(vt, nobuffer));
            jsonEqual('chars', json);
            done();
        });
    });

    it('range typeerror fontstack', function(done) {
        var glyphs = new fontnik.Glyphs();
        assert.throws(function() {
            glyphs.range(0, '0-256', fontnik.getRange(0, 256), function() {});
        }, /fontstack must be a string/);
        done();
    });

    it('range typeerror range', function(done) {
        var glyphs = new fontnik.Glyphs();
        assert.throws(function() {
            glyphs.range('Open Sans Regular', 0, fontnik.getRange(0, 256), function() {});
        }, /range must be a string/);
        done();
    });

    it('range typeerror chars', function(done) {
        var glyphs = new fontnik.Glyphs();
        assert.throws(function() {
            glyphs.range('Open Sans Regular', '0-256', 'foo', function() {});
        }, /chars must be an array/);
        done();
    });

    it('range typeerror callback', function(done) {
        var glyphs = new fontnik.Glyphs();
        assert.throws(function() {
            glyphs.range('Open Sans Regular', '0-256', fontnik.getRange(0, 256), '');
        }, /callback must be a function/);
        done();
    });

    it('range for fontstack with 0 matching fonts', function(done) {
        var glyphs = new fontnik.Glyphs();
        glyphs.range('doesnotexist', '0-256', fontnik.getRange(0, 256), function(err) {
            assert.ok(err);
            assert.equal("Error: Failed to find face 'doesnotexist' in font set 'doesnotexist'", err.toString());
            done();
        });
    });

    it('range for fontstack with 1 bad font', function(done) {
        var glyphs = new fontnik.Glyphs();
        glyphs.range('Open Sans Regular, doesnotexist', '0-256', fontnik.getRange(0, 256), function(err) {
            assert.ok(err);
            assert.equal("Error: Failed to find face 'doesnotexist' in font set 'Open Sans Regular, doesnotexist'", err.toString());
            done();
        });
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

describe('codepoints', function() {
    it('basic scanning: open sans', function() {
        var glyphs = new fontnik.Glyphs();
        var cp = glyphs.codepoints('Open Sans Regular');
        assert.equal(cp.length, 882);
    });
    it('basic scanning: fira sans', function() {
        var glyphs = new fontnik.Glyphs();
        var cp = glyphs.codepoints('Fira Sans Medium');
        assert.equal(cp.length, 789);
    });
    it('basic scanning: fira sans + open sans', function() {
        var glyphs = new fontnik.Glyphs();
        var cp = glyphs.codepoints('Fira Sans Medium, Open Sans Regular');
        assert.equal(cp.length, 1021);
    });
    it('invalid font face', function() {
        var glyphs = new fontnik.Glyphs();
        assert.throws(function() {
            glyphs.codepoints('foo-bar-invalid');
        });
    });
});

describe('faces', function() {
    it('list faces', function() {
        var faces = fontnik.faces();
        assert.deepEqual(faces, ['Fira Sans Medium', 'Open Sans Regular']);
    });
});
