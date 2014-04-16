{
  'targets': [
    {
      'target_name': 'fontserver',
      'type': 'executable',
      'sources': [
        'src/main.cpp',
        'src/harfbuzz_shaper.cpp',
        'src/itemizer.cpp',
        'src/scrptrun.cpp',
        'src/font_face.cpp',
        'src/font_face_set.cpp',
        'src/font_engine_freetype.cpp',
        'src/text_line.cpp',
        'src/font_set.cpp',
        'src/util.cpp',
        'src/distmap.c',
        'src/edtaa4func.c'
      ],
      'include_dirs': [
        '<!@(pkg-config freetype2 --cflags-only-I | sed s/-I//g)',
        '<!@(pkg-config icu-uc --cflags-only-I | sed s/-I//g)'
      ],
      'libraries': [
        '-lboost_system',
        '-lboost_filesystem',
        '<!@(pkg-config freetype2 --libs)',
        '<!@(pkg-config icu-uc --libs)',
        '<!@(pkg-config harfbuzz-icu --libs)',
        '<!@(pkg-config protobuf --libs)'
      ],
      'xcode_settings': {
          'MACOSX_DEPLOYMENT_TARGET': '10.8',
          'OTHER_CPLUSPLUSFLAGS': ['-std=c++11', '-stdlib=libc++', '-Wno-unused-variable'],
          'GCC_ENABLE_CPP_RTTI': 'YES',
          'GCC_ENABLE_CPP_EXCEPTIONS': 'YES'
      },
      'cflags_cc!': ['-fno-rtti', '-fno-exceptions'],
      'cflags_cc' : ['-std=c++11'],
      'cflags_c' : ['-std=c99'],
    }
  ]
}
