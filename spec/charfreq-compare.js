var modern = require('./cjk-modern.json');
var osm = require('./cjk-osm.json');

var diff = 0;
for (var k in osm) if (!modern[k]) diff++
console.log('%d chars diff (cjk-osm vs cjk-modern)', diff);
