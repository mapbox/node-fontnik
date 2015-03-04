var zlib = require('zlib');
var path = require('path');
var util = require('util');
var fontnik = require('./lib/fontnik.node');

module.exports = fontnik;
module.exports.range = range;
module.exports.getRange = getRange;

// Retrieve a range of glyphs as a pbf.
function range(options, callback) {
    'use strict';
    if (!options || typeof options.fontstack !== 'string') throw new Error('options.fontstack must be a string');

    var glyphs = new fontnik.Glyphs();
    glyphs.range(options.fontstack, options.start + '-' + options.end, getRange(options.start, options.end), gzip);

    function gzip(err) {
        if (err) return callback(err);
        var after = glyphs.serialize();
        zlib.gzip(after, callback);
    }
}

function getRange(start, end) {
    if (typeof start !== 'number') throw new Error('start must be a number from 0-65535');
    if (start < 0) throw new Error('start must be a number from 0-65535');
    if (typeof end !== 'number') throw new Error('end must be a number from 0-65535');
    if (end > 65535) throw new Error('end must be a number from 0-65535');
    if (start > end) throw new Error('start must be less than or equal to end');
    var range = [];
    for (var i = start; i <= end; i++) range.push(i);
    return range;
}
