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
module.exports.getRange = getRange;

// Retrieve a range of glyphs as a pbf.
function range(options, callback) {
    'use strict';
    options = options || {};
    options.fontstack = options.fontstack || 'Open Sans Regular';

    var glyphs = new fontserver.Glyphs();
    glyphs.range(options.fontstack, options.start + '-' + options.end, getRange(options.start, options.end), deflate);

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

function getRange(start, end) {
    if (typeof start !== 'number') throw new Error('start must be a number from 0-65533');
    if (start < 0) throw new Error('start must be a number from 0-65533');
    if (typeof end !== 'number') throw new Error('end must be a number from 0-65533');
    if (end > 65533) throw new Error('end must be a number from 0-65533');
    if (start > end) throw new Error('start must be less than or equal to end');
    var range = [];
    for (var i = start; i <= end; i++) range.push(i);
    return range;
}

