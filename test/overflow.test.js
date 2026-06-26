'use strict';

/* jshint node: true */

var test = require('tape');
var path = require('path');
var execFileSync = require('child_process').execFileSync;

// Regression test for the integer-overflow -> heap out-of-bounds write in
// RenderSDF. A crafted 680-byte font produces a glyph whose buffered
// dimensions are 65536x65536; the 32-bit bitmap size wraps to 0, the
// undersized buffer is then written past its end, and the worker process
// crashes with a memory fault. The glyph dimensions must be capped so that
// oversized glyphs are dropped instead of corrupting memory.
test('range: oversized glyph is dropped instead of crashing', function(t) {
    var worker = path.resolve(__dirname, 'overflow-worker.js');
    var stdout;
    try {
        stdout = execFileSync(process.execPath, [worker], {
            encoding: 'utf8',
            timeout: 60000
        });
    } catch (err) {
        t.fail('rendering crashed the worker process: signal=' + err.signal +
            ' status=' + err.status + (err.stderr ? '\n' + err.stderr : ''));
        t.end();
        return;
    }

    var result;
    try {
        result = JSON.parse(stdout.trim().split('\n').pop());
    } catch (e) {
        t.fail('could not parse worker output: ' + JSON.stringify(stdout));
        t.end();
        return;
    }

    t.ok(result.ok, 'range completed without error');
    t.equal(result.glyphWidth, 0, 'oversized glyph U+0041 is dropped (width 0)');
    t.equal(result.glyphHeight, 0, 'oversized glyph U+0041 is dropped (height 0)');
    t.notOk(result.hasBitmap, 'no bitmap emitted for the dropped glyph');
    t.end();
});
