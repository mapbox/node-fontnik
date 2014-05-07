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
            "<(SHARED_INTERMEDIATE_DIR)/vector_tile.pb.cc"
          ],
          'action': ['protoc','-Iproto/','--cpp_out=<(SHARED_INTERMEDIATE_DIR)/','./proto/vector_tile.proto']
        }
      ]
    },
    {
      'target_name': 'fontserver',
      'dependencies': [ 'action_before_build' ],
      'sources': [
        'src/font_face.cpp',
        'src/font_engine_freetype.cpp',
        'src/harfbuzz_shaper.cpp',
        'src/tile.cpp',
        'src/fontserver.cpp',
        'src/itemizer.cpp',
        'src/scrptrun.cpp',
        'src/font_face_set.cpp',
        'src/text_line.cpp',
        'src/font_set.cpp',
        'src/util.cpp',
        'src/distmap.c',
        'src/edtaa4func.c',
        '<(SHARED_INTERMEDIATE_DIR)/vector_tile.pb.cc'
      ],
      'include_dirs': [
        'src/',
        '<(SHARED_INTERMEDIATE_DIR)/',
        '<!@(pkg-config freetype2 --cflags-only-I | sed s/-I//g)',
        '<!@(pkg-config icu-uc --cflags-only-I | sed s/-I//g)',
        '<!@(pkg-config protobuf --cflags-only-I | sed s/-I//g)'
      ],
      'libraries': [
        '-lboost_system',
        '-lboost_filesystem',
        '<!@(pkg-config freetype2 --libs --static)',
        '<!@(pkg-config protobuf --libs --static)',
        '<!@(pkg-config harfbuzz-icu --libs --static)',
      ],
      'xcode_settings': {
          'MACOSX_DEPLOYMENT_TARGET': '10.8',
          'OTHER_CPLUSPLUSFLAGS': ['-Wshadow','-std=c++11', '-stdlib=libc++', '-Wno-unused-variable'],
          'GCC_ENABLE_CPP_RTTI': 'YES',
          'GCC_ENABLE_CPP_EXCEPTIONS': 'YES'
      },
      'cflags_cc!': ['-fno-rtti', '-fno-exceptions'],
      'cflags_cc' : ['-std=c++11','-Wshadow'],
      'cflags_c' : ['-std=c99'],
    },
    {
      'target_name': 'action_after_build',
      'type': 'none',
      'dependencies': [ '<(module_name)' ],
      'copies': [
          {
            'files': [ '<(PRODUCT_DIR)/<(module_name).node' ],
            'destination': '<(module_path)'
          }
      ]
    }
  ]
}
