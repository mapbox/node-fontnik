var fontserver = require('./build/Release/fontserver.node');
var zlib = require('zlib');
var path = require('path');
var fonts = path.join(path.dirname(module.filename), 'fonts');
process.env['FONTCONFIG_PATH'] = fonts;

module.exports = fontserver;

module.exports.convert = function(zdata, callback) {
    'use strict';
    var tile;

    zlib.inflate(zdata, inflated);

    function inflated(err, data) {
        if (err) return callback(err);
        tile = new fontserver.Tile(data);
        tile.simplify(simplified);
    }

    function simplified(err) {
        if (err) return callback(err);
        tile.shape('Open Sans, Jomolhari, Siyam Rupali, Alef, Arial Unicode MS', shaped);
    }

    function shaped(err) {
        if (err) return callback(err);
        var after = tile.serialize();
        zlib.deflate(after, callback);
    }
};

