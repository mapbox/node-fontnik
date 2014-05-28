[![Build Status](https://magnum.travis-ci.com/mapbox/fontserver.png?token=ctvz1otCksqcmNzRzzxa&branch=master)](https://magnum.travis-ci.com/mapbox/fontserver)

fontserver
----------
Is no longer a server, but a library that delivers a range of characters
rendered as SDFs (signed distance fields) in a protobuf.

## Installing

Make sure you have `boost`, `freetype`, `icu4c`, `harfbuzz` and `protobuf` installed. With homebrew, you can
type `brew install boost --c++11 --with-icu freetype icu4c --c++11 protobuf --c++11`. The makefile uses `pkg-config` to find these
libraries and links dynamically to them, so make sure that `pkg-config` can find
them.

```
npm install
```

## Usage

This is included in the following projects:

- `llmr`
- `tilestream-pro` (`gl` branch)
