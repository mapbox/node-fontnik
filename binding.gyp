{
  'includes': [ 'deps/common-freetype.gypi', 'deps/common-harfbuzz.gypi' ],
  'targets': [
    {
      'target_name': 'fontserver',
      'dependencies': [
        'deps/freetype.gyp:freetype',
        'deps/harfbuzz.gyp:harfbuzz'
      ],
      'sources': [
        'src/fontserver.cpp',
        'src/font.cpp',
        'src/shaping.cpp'
      ],
    }
  ]
}
