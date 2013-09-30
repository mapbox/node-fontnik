var fs = require('fs');
var fontserver = require('./');

var blob = fs.readFileSync('fonts/ArialUnicode.ttf');

var font = new fontserver.Font(blob);

// console.warn(fontserver.shape("لسعودية كسول الزنجبيل القط", font));
console.warn(fontserver.shape("MapBox", font));
