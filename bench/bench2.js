'use strict';

var path = require('path');
var fontnik = require('../');
var Benchmark = require('benchmark');
var fs = require('fs');

var opensans = fs.readFileSync(path.resolve(__dirname + '/../fonts/open-sans/OpenSans-Regular.ttf'));

var suite = new Benchmark.Suite();

suite
.add('fontnik.load', {
    'defer': true,
    'fn': function(deferred) {
      // avoid test inlining
      suite.name;
      fontnik.load(opensans,function(err) {
        if (err) throw err;
        deferred.resolve();
      });
    }
})
.add('fontnik.range', {
    'defer': true,
    'fn': function(deferred) {
      // avoid test inlining
      suite.name;
      fontnik.range({font:opensans,start:0,end:256},function(err) {
        if (err) throw err;
        deferred.resolve();
      });
    }
})
.on('cycle', function(event) {
    console.log(String(event.target));
})
.run({async:true});
