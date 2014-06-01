var cjk = {
    osm: require('./cjk-osm.json'),
    modern: require('./cjk-modern.json')
};

var tiles = [
    require('./fixtures/14.13718.6692.json'),
    require('./fixtures/14.13718.6693.json'),
    require('./fixtures/14.13719.6692.json'),
    require('./fixtures/14.13719.6693.json'),
    require('./fixtures/14.13345.7109.json'),
    require('./fixtures/14.13346.7109.json'),
    require('./fixtures/14.13345.7110.json'),
    require('./fixtures/14.13346.7110.json'),
    require('./fixtures/14.13486.6207.json'),
    require('./fixtures/14.13487.6207.json'),
    require('./fixtures/14.13486.6208.json'),
    require('./fixtures/14.13487.6208.json'),
];

var ranges = { osm:{}, modern:{} };

tiles.forEach(function(layers) {
    layers.forEach(function(l) {
        l.features.forEach(function(f) {
            if (!f.properties.name) return;
            var name = f.properties.name;
            for (var i = 0; i < name.length; i++) {
                var char = name.charCodeAt(i);
                ['osm','modern'].forEach(function(type) {
                    if (cjk[type][char]) {
                        ranges[type][cjk[type][char]] = true;
                    } else {
                        var start = Math.floor(char/256) * 256;
                        var range = start + '-' + (start + 255);
                        ranges[type][range] = true;
                    }
                });
            }
        });
    });
});

console.log('osm (%s ranges)', Object.keys(ranges.osm).length);
console.log(Object.keys(ranges.osm).sort());

console.log('modern (%s ranges)', Object.keys(ranges.modern).length);
console.log(Object.keys(ranges.modern).sort());
