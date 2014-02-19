var zlib = require('zlib');
var path = require('path');
var util = require('util');
var fs = require('fs');

// Fontserver fontconfig directories must be set in the conf file prior to
// require. Allow these to be passed in via FONTSERVER_FONTS env var.
var env_options = {};
if (process.env['FONTSERVER_FONTS']) env_options.fonts = process.env['FONTSERVER_FONTS'].split(';');

// Fontserver conf setup. Synchronous at require time.
fs.writeFileSync('/tmp/fontserver-fc.conf', conf(env_options));
process.env['FONTCONFIG_FILE'] = '/tmp/fontserver-fc.conf';

var fontserver = require('./build/Release/fontserver.node');

module.exports = fontserver;
module.exports.conf = conf;
module.exports.convert = convert;

// Convert a zlib deflated mapnik vector pbf to a gl pbf.
function convert(zdata, options, callback) {
    'use strict';
    options = options || {};
    options.fontstack = options.fontstack || 'Open Sans';

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

// Write a fontconfig XML configuration file.
function conf(options) {
    options = options || {};
    options.fonts = options.fonts || [path.resolve(__dirname + '/fonts')];

    var fontdirs = options.fonts.map(function(d) {
        return '<dir>' + d + '</dir>';
    }).join('');

    return util.format('<?xml version="1.0"?>\n\
<!DOCTYPE fontconfig SYSTEM "fonts.dtd">\n\
<fontconfig>\n\
    %s\n\
    <cachedir>/tmp/fontserver-fc-cache</cachedir>\n\
    <config></config>\n\
</fontconfig>\n', fontdirs);
}

