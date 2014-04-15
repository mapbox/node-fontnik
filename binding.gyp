{
  'targets': [
    {
      'target_name': 'action_before_build',
      'type': 'none',
      'hard_dependency': 1,
      'actions': [
        {
          'action_name': 'run_protoc_vector_tile',
          'inputs': [
            './proto/vector_tile.proto'
          ],
          'outputs': [
            "./src/vector_tile.pb.cc"
          ],
          'action': ['protoc','-Iproto/','--cpp_out=./src/','./proto/vector_tile.proto']
        }
      ]
    },
    {
      'target_name': 'fontserver',
      'sources': [
        'src/icu/scrptrun.cpp',
        'src/itemizer.cpp',
        'src/harfbuzz_shaper.cpp',
        'src/font_face.cpp',
        'src/font_face_set.cpp',
        'src/tile.cpp',
        'src/font_engine_freetype.cpp',
        'src/text_line.cpp',
        'src/fontserver.cpp',
        'src/clipper.cpp',
        'src/font_set.cpp',
        'src/util.cpp',
        'src/distmap.c',
        'src/edtaa4func.c',
        'src/vector_tile.pb.cc'
      ],
      'include_dirs': [
        '<!@(pkg-config freetype2 --cflags-only-I | sed s/-I//g)',
        '<!@(pkg-config icu-uc --cflags-only-I | sed s/-I//g)',
        '<!@(pkg-config harfbuzz-icu --cflags-only-I | sed s/-I//g)',
        '<!@(pkg-config protobuf --cflags-only-I | sed s/-I//g)'
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
