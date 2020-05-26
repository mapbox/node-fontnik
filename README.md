# node-fontnik

[![NPM](https://nodei.co/npm/fontnik.png?compact=true)](https://nodei.co/npm/fontnik/)
[![Build Status](https://travis-ci.org/mapbox/node-fontnik.svg?branch=master)](https://travis-ci.org/mapbox/node-fontnik)
[![codecov](https://codecov.io/gh/mapbox/node-fontnik/branch/master/graph/badge.svg)](https://codecov.io/gh/mapbox/node-fontnik)

A library that delivers a range of glyphs rendered as SDFs (signed distance fields) in a protocol buffer. We use these encoded glyphs as the basic blocks of font rendering in [Mapbox GL](https://github.com/mapbox/mapbox-gl-js). SDF encoding is superior to traditional fonts for our usecase in terms of scaling, rotation, and quickly deriving halos - WebGL doesn't have built-in font rendering, so the decision is between vectorization, which tends to be slow, and SDF generation.

The approach this library takes is to parse and rasterize the font with Freetype (hence the C++ requirement), and then generate a distance field from that rasterized image.

See also [TinySDF](https://github.com/mapbox/tiny-sdf), which is a faster but less precise approach to generating SDFs for fonts.

## [API](API.md)

## Installing

By default, installs binaries. On these platforms no external dependencies are needed.

- 64 bit OS X or 64 bit Linux
- Node.js v8-v14

Just run:

```
npm install
```

However, other platforms will fall back to a source compile: see [building from source](#building-from-source) for details.

## Building from source

```
npm install --build-from-source
```
Building from source should automatically install `boost`, `freetype` and `protozero` locally using [mason](https://github.com/mapbox/mason). These dependencies can be installed manually by running `./scripts/install_deps.sh`.

## Local testing

Run tests with

```
npm test
```

If you make any changes to the C++ files in the `src/` directory, you'll need to recompile the node bindings (`fontnik.node`) before testing locally:

```
make
```

See the `Makefile` for additional tasks you can run, such as `make coverage`.

## Background reading
- [Drawing Text with Signed Distance Fields in Mapbox GL](https://www.mapbox.com/blog/text-signed-distance-fields/)
- [State of Text Rendering](http://behdad.org/text/)
- [Pango vs HarfBuzz](http://mces.blogspot.com/2009/11/pango-vs-harfbuzz.html)
- [An Introduction to Writing Systems & Unicode](http://rishida.net/docs/unicode-tutorial/)
