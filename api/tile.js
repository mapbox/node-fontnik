var zlib = require('zlib');
var fs = require('fs');
var async = require('async');
var fontserver = require('./');


// var tile = new Tile(<uncompressed data as buffer>)
// tile.serialize() => <uncompressed data as buffer>
// tile.simplify(function(err) {})


function workflow(finalize) {
    // var data = fs.readFileSync('./tiles/14-8801-5373.vector.pbf'); // berlin
    var data = fs.readFileSync('./tiles/14-14553-6450.vector.pbf');
    // console.warn('SIZE initial compressed', data.length);
    zlib.inflate(data, function(err, data) {
        // console.warn('SIZE initial', data.length);
        if (err) throw err;
        var tile = new fontserver.Tile(data);

        // console.time('simplify');
        tile.simplify(function(err) {
            // console.timeEnd('simplify');
            if (err) throw err;

            // console.warn('SIZE simplified', tile.serialize().length);

            console.time('shape');
            tile.shape('Open Sans, Arial Unicode MS', function(err) {
                console.timeEnd('shape');
                if (err) throw err;

                // console.time('serialize');
                var after = tile.serialize();
                // console.timeEnd('serialize');
                // console.warn('SIZE after', after.length);

                zlib.deflate(after, function(err, compressed) {
                    if (err) throw err;
                    fs.writeFileSync('out.sdf', compressed);
                    // console.warn('SIZE after compressed', compressed.length);
                    finalize(null);
                });
            });
        });
    });
}



var count = 0;
async.whilst(
    function() { return count < 10; },
    function(callback) {
        count++;
        workflow(callback);
    },
    function(err) {
        console.warn(err, 'ok');
    }
);
