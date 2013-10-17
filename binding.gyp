{
  'targets': [
    {
      'target_name': 'fontserver',
      'sources': [
        'src/fontserver.cpp',
        'src/font.cpp',
        'src/tile.cpp',
        'src/shaping.cpp',
        'src/clipper.cpp',
        'src/sdf_renderer.cpp',
        'src/distmap.c',
        'src/edtaa4func.c',
        'src/globals.cpp',
        'src/metrics.pb.cc',
        'src/vector_tile.pb.cc'
      ],
      'include_dirs': [
        '<!@(pkg-config pangoft2 --cflags-only-I | sed s/-I//g)',
        '<!@(pkg-config protobuf --cflags-only-I | sed s/-I//g)',
      ],
      'libraries': [
        '<!@(pkg-config pangoft2 --libs)',
        '<!@(pkg-config protobuf --libs)'
      ],
      'xcode_settings': {
          'OTHER_CPLUSPLUSFLAGS': ['-std=c++11', '-Wno-unused-variable'],
          'GCC_ENABLE_CPP_RTTI': 'YES',
          'GCC_ENABLE_CPP_EXCEPTIONS': 'YES'
      },
      'cflags_cc!': ['-fno-rtti', '-fno-exceptions'],
      'cflags_cc' : ['-std=c++0x', '-Wno-unused-variable'],
      'cflags_c' : ['-std=c99'],
    }
  ]
}
