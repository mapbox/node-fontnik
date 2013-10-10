var fs = require('fs');
var zlib = require('zlib');
var Font = require('./font');

var fonts = exports.fonts = {};

exports.load = function(file, callback) {
    fs.readFile(file, function(err, data) {
        if (err) return callback(err);
        zlib.inflate(data, function(err, data) {
            if (err) return callback(err);

            var font = new Font(data);
            fonts[font.family] = fonts[font.family] || {};
            fonts[font.family][font.style] = font;

            callback(null);
        });
    });
};

exports.get = function(family, style, glyph) {
    if (!(family in fonts)) return;
    if (!(style in fonts[family])) return;
    return fonts[family][style].glyphs[glyph];
};
