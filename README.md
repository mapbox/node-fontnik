# node-fontnik

A library that delivers a range of glyphs rendered as SDFs (signed distance fields) in a protobuf.

[![NPM](https://nodei.co/npm/fontnik.png)](https://nodei.co/npm/fontnik/)

[![Build Status](https://secure.travis-ci.org/mapbox/node-fontnik.png)](https://travis-ci.org/mapbox/node-fontnik)

## Installing

By default, installs binaries. On these platforms no external dependencies are needed.

- 64 bit OS X or 64 bit Linux
- Node v0.10.x

Just run:

```
npm install
```

However, other platforms will fall back to a source compile: see [building from source](#building-from-source) for details.

## Building from source

Make sure you have `boost`, `freetype`, and `protobuf` installed. With [Homebrew](http://brew.sh/), you can
type `brew install boost --c++11 freetype protobuf --c++11`. The Makefile uses `pkg-config` to find these
libraries and links dynamically to them, so make sure that `pkg-config` can find
them.

```
npm install --build-from-source
```

## Background reading
- [State of Text Rendering](http://behdad.org/text/)
- [Pango vs HarfBuzz](http://mces.blogspot.com/2009/11/pango-vs-harfbuzz.html)
- [An Introduction to Writing Systems & Unicode](http://rishida.net/docs/unicode-tutorial/)
