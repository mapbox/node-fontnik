# 0.5.0

- Now using external https://github.com/mapbox/sdf-glyph-foundry
- Now only building binaries for node v4/v6
- Now publishing debug builds for linux
- Now publishing asan builds for linux
- Moved coverage reporting to codecov.io
- Fixed crash on font with null family name

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
