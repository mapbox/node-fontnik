var fs = require('fs');
var path = require('path');
var exec = require('child_process').exec;
var test = require('tape');
var queue = require('queue-async');
var mkdirp = require('mkdirp');

var bin_output = path.resolve(__dirname + '/bin_output');

test('setup', function(t) {
    mkdirp(bin_output, function(err) {
        t.error(err, 'setup');
        t.end();
    });
});

var registry_invalid = path.normalize(__dirname + '/fixtures/fonts-invalid');
var registry = path.normalize(__dirname + '/fixtures/fonts');


test('bin/build-glyphs', function(t) {
    var script = path.normalize(__dirname + '/../bin/build-glyphs'),
        font = path.normalize(__dirname + '/../fonts/open-sans/OpenSans-Regular.ttf'),
        dir = path.resolve(__dirname + '/bin_output');
    t.test('outputs expected', function(q) {
        exec([script, font, dir].join(' '), function(err, stdout, stderr) {
            q.error(err);
            if (!process.env.TOOLSET) q.error(stderr);
            fs.readdir(bin_output, function(err, files) {
                q.equal(files.length, 256, 'outputs 256 files');
                q.equal(files.indexOf('0-255.pbf'), 0, 'expected .pbf');
                q.equal(files.filter(function(f) {
                    return f.indexOf('.pbf') > -1;
                }).length, files.length, 'all .pbfs');
                q.end();
            })
        });
    });
    t.test('errors on invalid font', function(q) {
        exec([script, path.join(registry_invalid,'1c2c3fc37b2d4c3cb2ef726c6cdaaabd4b7f3eb9.ttf'), dir].join(' '), function(err, stdout, stderr) {
            q.ok(err);
            q.ok(err.message.indexOf('font does not have family_name') > -1);
            q.end();
        });
    });
    t.end();
});

test('bin/font-inspect', function(t) {
    var script = path.normalize(__dirname + '/../bin/font-inspect'),
        opensans = path.normalize(__dirname + '/fixtures/fonts/OpenSans-Regular.ttf'),
        firasans = path.normalize(__dirname + '/fixtures/fonts/FiraSans-Medium.ttf');

    t.test(' --face', function(q) {
        exec([script, '--face=' + opensans].join(' '), function(err, stdout, stderr) {
            q.error(err);
            if (!process.env.TOOLSET) q.error(stderr);
            q.ok(stdout.length, 'outputs to console');
            var output = JSON.parse(stdout);
            q.equal(output.length, 1, 'single face');
            q.equal(output[0].face, 'Open Sans Regular');
            q.ok(Array.isArray(output[0].coverage));
            q.equal(output[0].coverage.length, 882);
            q.end();
        });
    });

    t.test(' --register', function(q) {
        exec([script, '--register=' + registry].join(' '), function(err, stdout, stderr) {
            q.error(err);
            if (!process.env.TOOLSET) q.error(stderr);
            q.ok(stdout.length, 'outputs to console');
            var output = JSON.parse(stdout);
            q.equal(output.length, 2, 'both faces in register');
            q.equal(output[0].face, 'Fira Sans Medium', 'Fira Sans Medium');
            q.ok(Array.isArray(output[0].coverage), 'codepoints array');
            q.equal(output[1].face, 'Open Sans Regular', 'Open Sans Regular');
            q.ok(Array.isArray(output[1].coverage), 'codepoints array');
            q.end();
        });
    });

    t.test(' --register --verbose', function(q) {
        exec([script, '--verbose', '--register=' + registry].join(' '), function(err, stdout, stderr) {
            q.error(err);
            q.ok(stderr.length, 'writes verbose output to stderr');
            if (!process.env.TOOLSET) {
                q.equal(stderr.indexOf('resolved'), 0);
                var verboseOutput = JSON.parse(stderr.slice(9).trim().replace(/'/g, '"'));
                t.equal(verboseOutput.length, 2);
                t.equal(verboseOutput.filter(function(f) { return f.indexOf('.ttf') > -1; }).length, 2);
            }
            q.ok(stdout.length, 'writes codepoints output to stdout');
            q.ok(JSON.parse(stdout));
            q.end();
        });
    });

    t.test(' --register --verbose', function(q) {
        exec([script, '--verbose', '--register=' + registry_invalid].join(' '), function(err, stdout, stderr) {
            q.ok(err);
            q.ok(stderr.indexOf('font does not have family_name or style_name') > -1);
            q.end();
        });
    });

    t.end();
});

test('teardown', function(t) {
    var q = queue();

    fs.readdir(bin_output, function(err, files) {
        files.forEach(function(f) {
            q.defer(fs.unlink, path.join(bin_output, '/', f));
        });

        q.awaitAll(function(err) {
            t.error(err, 'teardown');
            fs.rmdir(bin_output, function(err) {
                t.error(err, 'teardown');
                t.end();
            });
        });
    });
});
