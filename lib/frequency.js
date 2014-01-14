var fs = require('fs');
var async = require('async');
var zlib = require('zlib');
var Protobuf = require('pbf');
var VectorTile = require('./vectortile');
var fontserver = require('../');

var stack = 'Open Sans, Arial Hebrew, Arial Unicode MS, sans serif';

var fonts = {};

function showStats() {
    for (var family in fonts) {
        for (var style in fonts[family]) {
            var list = [];
            for (var glyph in fonts[family][style]) {
                list.push({ glyph: glyph, count: fonts[family][style][glyph] });
            }
            list.sort(countSort);
            console.warn(family, style, list);
        }
    }
    function countSort(a, b) {
        return b.count - a.count;
    }
}

process.on('SIGINT', function() {
    showStats();
    process.exit();
});

function loadTile(filename, done) {
    fs.readFile(filename, function(err, data) {
        if (err) return done(err);
        zlib.inflate(data, function(err, data) {
            if (err) return done(err);
            var tile = new VectorTile(new Protobuf(data));

            var tile_fonts = {}, family, style, j;

            for (var name in tile.layers) {
                var layer = tile.layers[name];
                for (var i = 0; i < layer.length; i++) {
                    var feature = layer.feature(i);
                    var label = feature.name;

                    if (!label) continue;

                    if (label.indexOf('&') >= 0) {
                        label = label.replace(/&/g, '&amp;');
                    }

                    var shaped = fontserver.shape(label, stack);
                    for (j = 0; j < shaped.length; j++) {
                        var info = shaped[j];
                        if (!(info.face.family in tile_fonts)) tile_fonts[info.face.family] = {};
                        family = tile_fonts[info.face.family];
                        if (!(info.face.style in family)) family[info.face.style] = [];
                        style = family[info.face.style];
                        if (style.indexOf(info.glyph) < 0) style.push(info.glyph);
                    }
                }
            }

            for (family in tile_fonts) {
                if (!(family in fonts)) fonts[family] = {};
                var tile_family = tile_fonts[family];
                for (style in tile_family) {
                    var tile_style = tile_family[style];
                    if (!(style in fonts[family])) fonts[family][style] = {};
                    for (j = 0; j < tile_style.length; j++) {
                        var glyph = tile_style[j];
                        if (!(glyph in fonts[family][style])) fonts[family][style][glyph] = 0;
                        fonts[family][style][glyph]++;
                    }
                }
            }

            process.stderr.write('.');

            done(null);
        });
    });
}

var files = fs.readdirSync('./tiles');

async.mapLimit(files, 16, function(file, done) {
    loadTile('./tiles/' + file, function(err, tile) {
        // console.warn(tile);
        done();
    });
}, function(err, results) {
    if (err) throw err;
    showStats();
    console.warn('ok');
});
