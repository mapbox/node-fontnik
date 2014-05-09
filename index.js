var zlib = require('zlib');
var path = require('path');
var util = require('util');
var fontserver = require('./lib/fontserver.node');

// Fontserver directories must be set in the conf file prior to require.
// Allow these to be passed in via FONTSERVER_FONTS env var.
var env_options = {};
if (process.env['FONTSERVER_FONTS']) env_options.fonts = process.env['FONTSERVER_FONTS'].split(';');

// Fontserver conf setup. Synchronous at require time.
conf(env_options);

module.exports = fontserver;
module.exports.convert = convert;

// Convert a zlib deflated mapnik vector pbf to a gl pbf.
function convert(zdata, options, callback) {
    'use strict';
    options = options || {};
    options.fontstack = options.fontstack || 'Open Sans Regular';

    var tile;

    zlib.inflate(zdata, inflated);

    function inflated(err, data) {
        if (err) return callback(err);
        tile = new fontserver.Tile(data);
        tile.shape(options.fontstack, shaped);
    }

    function shaped(err) {
        if (err) return callback(err);
        var after = tile.serialize();
        zlib.deflate(after, callback);
    }
}

// Retrieve a range of glyphs as a pbf.
function range(options, callback) {
    'use strict';
    options = options || {};
    options.fontstack = options.fontstack || 'Open Sans Regular';

    var tile = new fontserver.Tile();
    tile.range(options.fontstack, options.start, options.end, deflate);

    function deflate(err) {
        if (err) return callback(err);
        var after = tile.serialize();
        zlib.deflate(after, callback);
    }
}

// Register fonts in FreeType.
function conf(options) {
    options = options || {};
    options.fonts = options.fonts || [path.resolve(__dirname + '/fonts')];

    options.fonts.forEach(function(d) {
        fontserver.register_fonts(d, {recurse: true});
    });
}
