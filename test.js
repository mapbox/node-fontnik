var fontserver = require('./');

console.time('shape');
// for (var i = 0; i < 10000; i++) {
var shaped = fontserver.shape("Oderberger Straße", 'Open Sans, Arial Unicode MS');
// }
console.timeEnd('shape');
console.warn(shaped);

// console.warn(fontserver.shape("MapBox", font));
console.warn('ok');