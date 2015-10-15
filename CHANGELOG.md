# 0.3.3

* Fix bounds for short ranges. #91

# 0.2.6

* Truncate at Unicode point 65535 (0xFFFF) instead of 65533.

# 0.2.3

* Calling .codepoints() on an invalid font will throw a JavaScript
  error rather than running into an abort trap.
