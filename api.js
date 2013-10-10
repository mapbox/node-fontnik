

fontserver.glyphMetrics('Open Sans Regular', function(err, metrics) {
    zlib.deflate(metrics);
    send(metrics);
});

style = {
    layername: ['fontstack', 'fontstack']
};

zlib.inflate(tile);
fontserver.shapeTile(tile, style, function(err, tile) {
    zlib.deflate(tile);
    send(tile);
});
