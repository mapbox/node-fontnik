# 1.0.0

* `.codepoints()` now returns an object with `faces` array of family_name and
style_name metadata, and `codepoints` array of unicode point coverage.

# 0.2.3

* Calling .codepoints() on an invalid font will throw a JavaScript
  error rather than running into an abort trap.
