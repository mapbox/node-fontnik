const { join } = require('path');
const { existsSync } = require('fs');

const mod = 'fontnik.node';
const candidates = [
    join(__dirname, 'prebuilds', `${process.platform}-${process.arch}`, mod),
    join(__dirname, 'build', mod),
];
const path = candidates.find(existsSync);
if (!path) {
    throw new Error(`fontnik native module not found for ${process.platform}-${process.arch}; run \`npm run rebuild\``);
}
module.exports = require(path);
