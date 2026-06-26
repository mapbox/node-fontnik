'use strict';

/**
 * npm install hook: ensure a native fontnik binary exists for this platform.
 *
 * Skips work when prebuilds/<platform>-<arch>/fontnik.node is already present
 * (shipped in the npm package or produced by a prior `npm run rebuild`).
 * Otherwise compiles from source locally; in CI, missing prebuilds is fatal.
 */

const { join } = require('path');
const { existsSync } = require('fs');
const { execSync } = require('child_process');

const root = join(__dirname, '..');
const mod = 'fontnik.node';
const prebuild = join(root, 'prebuilds', `${process.platform}-${process.arch}`, mod);

if (existsSync(prebuild)) {
    process.exit(0);
}

if (process.env.CI === 'true' || process.env.CI === '1') {
    console.error('No prebuilt fontnik binary for this platform; run npm run rebuild in CI');
    process.exit(1);
}

execSync('npm run rebuild', { cwd: root, stdio: 'inherit' });
