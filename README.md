# node-fontnik

A library that delivers a range of glyphs rendered as SDFs (signed distance fields) in a protocol buffer. We use these encoded glyphs as the basic blocks of font rendering in [Mapbox GL](https://github.com/mapbox/mapbox-gl-js). SDF encoding is superior to traditional fonts for our usecase in terms of scaling, rotation, and quickly deriving halos - WebGL doesn't have built-in font rendering, so the decision is between vectorization, which tends to be slow, and SDF generation.

The approach this library takes is to parse and rasterize the font with Freetype (hence the C++ requirement), and then generate a distance field from that rasterized image.

See also [TinySDF](https://github.com/mapbox/tiny-sdf), which is a faster but less precise approach to generating SDFs for fonts.

## [API](API.md)

## Installing

Prebuilt binaries are included in the npm package for:

- macOS arm64 (`darwin-arm64`)
- macOS x64 (`darwin-x64`)
- Linux x64 (`linux-x64`)
- Linux arm64 (`linux-arm64`)

```
npm install
```

On other platforms, or when no matching prebuild exists, `npm install` compiles from source via CMake (see below). Node.js 20 or later is required.

## Building from source

Install system dependencies:

**macOS**

```
brew install cmake ninja boost zlib
```

**Linux (Debian/Ubuntu)**

```
sudo apt-get install cmake ninja-build zlib1g-dev libboost-dev
```

Then build:

```
npm run rebuild
```

N-API headers (`node-addon-api`, `node-api-headers`), `protozero`, and FreeType are fetched automatically by CMake at configure time.

## Local testing

```
npm test
```

After changing C++ sources in `src/`, rebuild the native module:

```
make
```

See the `Makefile` for `coverage`, `sanitize`, `tidy`, `format`, and Linux Docker testing (`make test-linux`).

### Linux builds via Docker

To mirror CI on a Mac (or any host with Docker), build and test inside Ubuntu 22.04 containers matching GHA:

```
./scripts/docker-linux-test.sh              # linux-x64 + linux-arm64, Release
./scripts/docker-linux-test.sh x64 Debug    # single arch / build type
```

Release builds write `prebuilds/linux-<arch>/fontnik.node` directly (same as local `npm run rebuild`).

Or `make test-linux` / `make test-linux-x64`. Use `--rebuild-image` after changing `scripts/docker/Dockerfile`.

## Release

See [DEV.md](./DEV.md).

## Background reading
- [Drawing Text with Signed Distance Fields in Mapbox GL](https://www.mapbox.com/blog/text-signed-distance-fields/)
- [State of Text Rendering](http://behdad.org/text/)
- [Pango vs HarfBuzz](http://mces.blogspot.com/2009/11/pango-vs-harfbuzz.html)
- [An Introduction to Writing Systems & Unicode](http://rishida.net/docs/unicode-tutorial/)
