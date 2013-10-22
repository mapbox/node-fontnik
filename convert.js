var path = require('path');
var fonts = path.join(path.dirname(module.filename), 'fonts');
process.env['FONTCONFIG_PATH'] = fonts;

var zlib = require('zlib');
var request = require('request');
var fs = require('fs');
var mkdirp = require('mkdirp');
var async = require('async');
var fontserver = require('./');


function loadTile(z, x, y, callback) {
    var filename = './tiles/' + z + '-' + x + '-' + y + '.vector.pbf';
    fs.readFile(filename, function(err, data) {
        if (err) {
            request({
                url: 'http://api.tiles.mapbox.com/v3/mapbox.mapbox-streets-v4/' + z + '/' + x + '/' + y + '.vector.pbf',
                encoding: null
            }, function(err, res, data) {
                if (err) {
                    callback(err);
                } else {
                    zlib.inflate(data, function(err, uncompressed) {
                        if (err) {
                            callback(err);
                        } else {
                            fs.writeFile(filename, data, function(err) {
                                callback(err, uncompressed);
                            });
                        }
                    });
                }
            });
        } else {
            zlib.inflate(data, function(err, uncompressed) {
                if (err) {
                    console.warn('zlib inflate failed for %d/%d/%d. redownloading...', z, x, y);
                    fs.unlink(filename, function(err) {
                        if (err) {
                            callback(err);
                        } else {
                            loadTile(z, x, y, callback);
                        }
                    });
                } else {
                    callback(null, uncompressed);
                }
            });
        }
    });
}


function convertTile(z, x, y, callback) {
    var tile;
    async.waterfall([
        function(callback) {
            loadTile(z, x, y, callback);
        },
        function(data, callback) {
            tile = new fontserver.Tile(data);
            tile.simplify(callback);
        },
        function(callback) {
            tile.shape('Open Sans, Jomolhari, Siyam Rupali, Alef, Arial Unicode MS', callback);
        },
        function(callback) {
            var after = tile.serialize();
            zlib.deflate(after, callback);
        }, function(data, callback) {
            fs.writeFile('./tiles-processed/' + z + '-' + x + '-' + y + '.vector.pbf', data, callback);
        }
    ], callback);
}

async.each(['tiles-processed'], mkdirp, function(err) {
    if (err) throw err;

    var pattern = /^(\d+)-(\d+)-(\d+)\.vector\.pbf$/;
    var files = fs.readdirSync('tiles').map(function(tile) {
        var matches = tile.match(pattern);
        if (matches) return { z: +matches[1], x: +matches[2], y: +matches[3] };
    }).filter(function(tile) {
        return tile;
    });
    files.sort(function(a, b) {
        if (a.z != b.z) return a.z - b.z;
        if (a.x != b.x) return a.x - b.x;
        return a.y - b.y;
    });

    async.eachLimit(files, 8, function(tile, callback) {
        convertTile(tile.z, tile.x, tile.y, function(err) {
            if (err) console.warn(tile, err.stack);
            callback(null);
        });
    }, function(err) {
        if (err) console.warn(err.stack);
        console.warn('done');
    });
});

