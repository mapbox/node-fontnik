var fs = require('fs');
var fontserver = require('./');

var blob = fs.readFileSync('fonts/ArialUnicode.ttf');

var font = new fontserver.Font(blob);

// console.warn(font[0]);

console.warn(fontserver.shape("لسعودية كسول الزنجبيل القط", font));

// for (var i = 0; i < font.length; i++) {
//     console.warn(font[i]);
// }