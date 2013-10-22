var path = require('path');
var fonts = path.join(path.dirname(module.filename), 'fonts');
process.env['FONTCONFIG_PATH'] = fonts;

var express = require('express');
var zlib = require('zlib');
var request = require('request');
var fs = require('fs');
var mkdirp = require('mkdirp');
var async = require('async');
var fontserver = require('./');

var app = express();

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
                    fs.writeFile(filename, data);
                    callback(null, data);
                }
            });
        } else {
            callback(null, data);
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
            zlib.inflate(data, callback);
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
            fs.writeFile('./tiles-processed/' + z + '-' + x + '-' + y + '.vector.pbf', data, function(err) {
                callback(err, data);
            });
        }
    ], callback);
}

app.get('/gl/tiles/:z(\\d+)-:x(\\d+)-:y(\\d+).vector.pbf', function(req, res) {
    var x = req.params.x, y = req.params.y, z = req.params.z;

    fs.readFile('./tiles-processed/' + z + '-' + x + '-' + y + '.vector.pbf', function(err, data) {
        if (err) {
            convertTile(z, x, y, send);
        } else {
            send(null, data);
        }
    });

    function send(err, compressed) {
        if (err) {
            console.error(err.stack);
            res.send(500, err.message);
        } else {
            res.setHeader('Cache-Control', 'max-age-86400');
            res.setHeader('Content-Type', 'application/x-vectortile');
            res.setHeader('Content-Encoding', 'deflate');
            res.setHeader('Content-Length', compressed.length);
            res.send(200, compressed);
        }
    }
});

app.use('/gl', express.static(__dirname + '/html'));

app.get('/', function(req, res) {
    res.redirect('/gl/');
});

async.each(['tiles', 'tiles-processed'], mkdirp, function(err) {
    if (err) throw err;
    app.listen(3000);
    console.log('Listening on port 3000');
});

