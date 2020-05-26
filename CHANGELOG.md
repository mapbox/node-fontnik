# 0.6.0

- Adds node v12 and v14 support
- Dropped node v4 and v6 support
- Adds `fontnik.composite`
- Drops `libprotobuf` dependency, uses `protozero` instead
- Requires c++14 compatible compiler
- Binaries are published using clang++ 10.0.0

# 0.5.2

- Adds .npmignore to keep downstream node_modules small.

# 0.5.1

- Stopped bundling node-pre-gyp
- Added support for node v8 and v10
- Various performance optimizations and safety checks

# 0.5.0

- Fixed crash on font with null family name
- Optimized the code to reduce memory allocations
- Now using external https://github.com/mapbox/sdf-glyph-foundry
- Now only building binaries for node v4/v6
- Now publishing debug builds for linux
- Now publishing asan builds for linux
- Upgraded from boost 1.62.0 -> 1.63.0
- Upgraded from freetype 2.6 -> 2.7.1
- Upgraded from protobuf 2.6.1 -> 3.2.0
- Moved coverage reporting to codecov.io

# 0.4.8

- Bundles `mkdirp` to avoid an npm@2 bug when using `bundledDependencies` with `devDependencies`.

# 0.4.7

- Upgrades to a modern version of Mason.

# 0.4.6

- Adds prepublish `npm ls` script to prevent publishing without `bundledDependencies`.

# 0.4.5

- Fixes Osaka range segfault.

# 0.4.4

- Fix initialization of `queue-async` in `bin/build-glyphs`.

# 0.4.3

- Handle `ft_face->style_name` null value in `RangeAsync`.

# 0.4.2

- Handle `ft_face->style_name` null value in `LoadAsync`.

# 0.4.1

- Publish Node.js v5.x binaries.
- Autopublish binaries on git tags.

# 0.4.0

- Fixes bounds for short ranges.
- Fixes Travis binary publishing.
- Adds Node.js v4.x support.

# 0.2.6

- Truncate at Unicode point 65535 (0xFFFF) instead of 65533.

# 0.2.3

- Calling .codepoints() on an invalid font will throw a JavaScript
  error rather than running into an abort trap.
