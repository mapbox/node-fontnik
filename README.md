[![Build Status](https://magnum.travis-ci.com/mapbox/fontnik.png?token=ctvz1otCksqcmNzRzzxa&branch=master)](https://magnum.travis-ci.com/mapbox/fontnik)

# fontnik

A library that delivers a range of characters rendered as SDFs (signed distance fields) in a protobuf.

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
