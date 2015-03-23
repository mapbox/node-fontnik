'use strict';

var path = require('path');
var fontnik = require('../');
var queue = require('queue-async');
var fs = require('fs');

// https://gist.github.com/mourner/96b1335c6a43e68af252
// https://gist.github.com/fengmk2/4345606
function now() {
    var hr = process.hrtime();
    return hr[0] + hr[1] / 1e9;
}

function bench(opts,cb) {
    var q = queue(opts.concurrency);
    var start = now();
    for (var i = 1; i <= opts.iterations; i++) {
        q.defer.apply({},opts.args);
    }
    q.awaitAll(function(error, results) {
        var seconds = now() - start;
        console.log(opts.name, Math.round(opts.iterations / (seconds)),'ops/sec',opts.iterations,opts.concurrency);
        return cb();
    });
}

function main() {
    var opensans = fs.readFileSync(path.resolve(__dirname + '/../fonts/open-sans/OpenSans-Regular.ttf'));

    var suite = queue(1);
    suite.defer(bench, {
        name:"fontnik.load",
        args:[fontnik.load,opensans],
        iterations: 10,
        concurrency: 10
    });
    suite.defer(bench, {
        name:"fontnik.range",
        args:[fontnik.range,{font:opensans,start:0,end:256}],
        iterations: 1000,
        concurrency: 100
    });
    suite.awaitAll(function(err) {
        if (err) throw err;
    })
}

main();
