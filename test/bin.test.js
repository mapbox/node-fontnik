var fs = require('fs');
var path = require('path');
var exec = require('child_process').exec;
var test = require('tape');
var queue = require('queue-async');
var mkdirp = require('mkdirp');

var bin_output = path.resolve(__dirname + '/bin_output');

var buffsizes = ['default', 100, 256, 512, 1024, 3000, 65536];

test('setup', function(t) {
    buffsizes.forEach(function(buffsize){
        t.test(' '+buffsize+' buffer size', function(q) {
            mkdirp(bin_output+'/'+buffsize, function(err) {
                    q.error(err, 'setup '+buffsize);
                    q.end();
                });
        });
    });
});

test('bin/build-glyphs', function(t) {
    var script = path.normalize(__dirname + '/../bin/build-glyphs'),
        font = path.normalize(__dirname + '/../fonts/open-sans/OpenSans-Regular.ttf'),
        dir = path.resolve(__dirname + '/bin_output');

    buffsizes.forEach(function(buffsize){
        t.test(' '+buffsize+' buffer size', function(q) {
            var params = [script, font, dir+'/'+buffsize];
            if(buffsize !== 'default')
                params.push(buffsize);

            exec(params.join(' '), function(err, stdout, stderr) {
                q.error(err);
                q.error(stderr);
                fs.readdir(dir+'/'+buffsize, function(err, files) {
                    var buffsizeno = parseInt(buffsize)||256;
                    var filesno = Math.ceil(65536/buffsizeno);

                    q.equal(files.length, filesno, 'outputs '+filesno+' files');

                    //fist file ( for example 0-255.pbf )
                    q.equal(files.indexOf('0-'+(buffsizeno-1)+'.pbf'), 0, 'expected first .pbf');

                    var start = (filesno-1)*buffsizeno;
                    var end   = Math.min(filesno*buffsizeno-1, 65535);
                    //there mustbe last file ( for example 65535 )
                    q.equal(files.indexOf(start+'-'+end+'.pbf')>=0, true, 'expected last '+start+'-'+end+'.pbf');

                    //all files must have valid format
                    q.equal(files.filter(function(f) {
                        var is;

                        //must match NUMBER-NUMBER.pbf
                        f.replace(/^([0-9]+)\-([0-9]+)\.pbf$/g, function(all, s, e){
                            is = true;
                            s = parseInt(s);
                            e = parseInt(e);

                            //start range must be divided by buffer size
                            if(s%buffsizeno !== 0) is = false;

                            //end range + 1 must be divided by buffer size
                            if(e !== 65535 && (e+1)%buffsizeno !== 0) is = false;

                            //start and and range must be between 0-65535
                            if(
                                ( s < 0 || s > 65535 )
                                ||
                                ( e < 0 || e > 65535 )
                            ){
                                is = false;
                            }
                        });


                        return is;
                    }).length, files.length, 'valid .pbfs names');
                    q.end();
                })
            });
        });
    });


});

test('bin/font-inspect', function(t) {
    var script = path.normalize(__dirname + '/../bin/font-inspect'),
        opensans = path.normalize(__dirname + '/fixtures/fonts/OpenSans-Regular.ttf'),
        firasans = path.normalize(__dirname + '/fixtures/fonts/FiraSans-Medium.ttf'),
        registry = path.normalize(__dirname + '/fixtures/fonts');

    t.test(' --face', function(q) {
        exec([script, '--face=' + opensans].join(' '), function(err, stdout, stderr) {
            q.error(err);
            q.error(stderr);
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
            q.error(stderr);
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
            q.equal(stderr.indexOf('resolved'), 0);
            var verboseOutput = JSON.parse(stderr.slice(9).trim().replace(/'/g, '"'));
            t.equal(verboseOutput.length, 2);
            t.equal(verboseOutput.filter(function(f) { return f.indexOf('.ttf') > -1; }).length, 2);
            q.ok(stdout.length, 'writes codepoints output to stdout');
            q.ok(JSON.parse(stdout));
            q.end();
        });
    });

    t.end();
});

test('teardown', function(t) {
    var q1 = queue();

    var dirs = buffsizes.map(function(v){return path.join(bin_output, '/', v+"");});
    dirs.forEach(function(dir){
        fs.readdirSync(dir).forEach(function(f) {
            q1.defer(fs.unlink, path.join(dir, '/', f));
        });
    });
    q1.awaitAll(function(err) {
        var q2 = queue();

        dirs.forEach(function(dir){
            q2.defer(fs.rmdir, dir);
        });

        q2.awaitAll(function(err) {
            t.error(err, 'teardown');
            fs.rmdir(bin_output, function(err) {
                t.error(err, 'teardown');
                t.end();
            });
        });
    });
});
