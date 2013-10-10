var fs = require('fs');
var glyphcache = require('./glyphcache');
var fontserver = require('../');
var _ = require('./underscore');
var async = require('async');
var Protobuf = require('./protobuf');
var Shaping = require('./shaping');
var zlib = require('zlib');
var VectorTile = require('./vectortile');

var fonts = [
    require.resolve('../sdf/ArialUnicode.sdf'),
    require.resolve('../sdf/OpenSans-Regular.sdf')
];

var stacks = [
    'Open Sans, Arial Hebrew, Arial Unicode MS, sans serif'
];

function loadFonts(done) {
    async.map(fonts, glyphcache.load, function(err) {
        done(err);
    });
}

function loadTile(filename, done) {
    fs.readFile(filename, function(err, data) {
        if (err) return done(err);
        zlib.inflate(data, function(err, data) {
            if (err) return done(err);
            var tile = new VectorTile(new Protobuf(data));
            done(null, tile);
        });
    });
}

function loadLabels(tile, done) {
    var labels = [];
    for (var name in tile.layers) {
        var layer = tile.layers[name];
        for (var i = 0; i < layer.length; i++) {
            var feature = layer.feature(i);
            var label = feature.name;

            if (!label) continue;

            if (label.indexOf('&') >= 0) {
                label = label.replace(/&/g, '&amp;');
            }

            for (var j = 0; j < label.length; j++) {
                if (label.charCodeAt(j) > 255) {
                    if (labels.indexOf(label) < 0) {
                        labels.push(label);
                    }
                    break;
                }
            }
        }
    }
    console.warn(labels);
    done(null, labels);
}

function shapeLabels(label_source, done) {
    console.time('shaping');
    var face_ids = [];
    var faces = {};
    var face_count = 0;

    var labels = [];

    for (var j = 0; j < label_source.length; j++) {
        var label_text = label_source[j];

        var shaped = fontserver.shape(label_text, stacks[0]);

        var label = {
            text: label_text,
            stack: 0,
            faces: [],
            glyphs: [],
            x: [],
            y: []
        };

        // Build faces object.
        for (var i = 0; i < shaped.length; i++) {
            var info = shaped[i];
            var key = info.face.family + '#' + info.face.style;

            if (!(key in faces)) {
                faces[key] = {
                    family: info.face.family,
                    style: info.face.style,
                    id: face_count++,
                    glyphs: {}
                };
                faces[faces[key].id] = faces[key];
                faces.length = face_count;
            }

            if (!(info.glyph in faces[key].glyphs)) {
                var glyph = glyphcache.get(info.face.family, info.face.style, info.glyph);
                if (glyph) {
                    glyph.id = info.glyph;
                    // console.warn('cached', glyph);
                    faces[key].glyphs[info.glyph] = glyph;
                }
                else {
                    faces[key].glyphs[info.glyph] = info.face[info.glyph];
                    // console.warn('computed', faces[key].glyphs[info.glyph]);
                }
            }

            label.faces.push(faces[key].id);
            label.glyphs.push(info.glyph);
            label.x.push(Math.round(info.x));
            label.y.push(Math.round(info.y));
        }

        labels.push(label);
    }

    done(null, {
        stacks: stacks,
        faces: _.toArray(faces),
        labels: labels
    });
}

function startTime(name) {
    return function() {
        console.time(name);
        var args = _.toArray(arguments);
        var done = args.pop();
        args.unshift(null);
        done.apply(this, args);
    };
}

function endTime(name) {
    return function() {
        console.timeEnd(name);
        var args = _.toArray(arguments);
        var done = args.pop();
        args.unshift(null);
        done.apply(this, args);
    };
}

async.waterfall([
    startTime('all'),
    loadFonts,
    function(done) {
        loadTile(require.resolve('../tiles/14-8800-5373.vector.pbf'), done);
    },
    startTime('loading'),
        loadLabels,
    endTime('loading'),
    startTime('shaping'),
        shapeLabels,
    endTime('shaping'),
    startTime('encoding'),
        function(result, done) {
            var pbf = new Shaping();
            pbf.writeRepeated('TaggedString', 1, result.stacks);
            pbf.writeRepeated('Faces', 2, result.faces);
            pbf.writeRepeated('Labels', 3, result.labels);
            var buffer = pbf.finish();
    
            fs.writeFile('./out.sdf', buffer, done);
        },
    endTime('encoding'),
    endTime('all')
], function(err) {
    if (err) throw err;
    console.warn('ok');
});

