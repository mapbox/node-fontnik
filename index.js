var zlib = require('zlib');
var path = require('path');
var util = require('util');
var fontnik = require('./lib/fontnik.node');

// Fontnik directories must be set in the conf file prior to require.
// Allow these to be passed in via FONTNIK_FONTS env var.
var env_options = {};
if (process.env['FONTNIK_FONTS']) env_options.fonts = process.env['FONTNIK_FONTS'].split(';');

// Fontnik conf setup. Synchronous at require time.
conf(env_options);

module.exports = fontnik;
module.exports.range = range;
module.exports.getRange = getRange;

// Retrieve a range of glyphs as a pbf.
function range(options, callback) {
    'use strict';
    options = options || {};
    options.fontstack = options.fontstack || 'Open Sans Regular';
    options.range = options.range || options.start + '-' + options.end;
    options.chars = options.chars || getRange(options.start, options.end);

    var glyphs = new fontnik.Glyphs();
    glyphs.range(options.fontstack, options.range, options.chars, deflate);

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
        fontnik.register_fonts(d, {recurse: true});
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

