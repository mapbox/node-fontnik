var fontserver = require('./');
var zlib = require('zlib');

var font = new fontserver.Font('Arial Unicode MS');

var metrics = font.metrics;
console.warn('uncompressed', metrics.length);
zlib.deflate(metrics, function(err, data) {
    if (err) throw err;
    console.warn('compressed', data.length);
    console.warn('ok');
});
