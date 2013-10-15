

var metrics = new Font('Open Sans Regular').metrics;
zlib.deflate(metrics);
send(metrics);

style = {
    layername: ['fontstack', 'fontstack']
};

zlib.inflate(tile);
fontserver.shapeTile(tile, style, function(err, tile) {
});

tile.shape(style, function(err, buffer) {
    zlib.deflate(tile);
    send(tile);
});
