var modern = require('./expected/cjk-modern.json');
var osm = require('./expected/cjk-osm.json');

var diff = 0;
for (var k in osm) if (!modern[k]) diff++
console.log('%d chars diff (cjk-osm vs cjk-modern)', diff);
