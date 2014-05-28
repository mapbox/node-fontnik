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
module.exports.range = range;

// Retrieve a range of glyphs as a pbf.
function range(options, callback) {
    'use strict';
    options = options || {};
    options.fontstack = options.fontstack || 'Open Sans Regular';

    var glyphs = new fontserver.Glyphs();
    glyphs.range(options.fontstack, options.start, options.end, deflate);

    function deflate(err) {
        if (err) return callback(err);
        var after = glyphs.serialize();
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
