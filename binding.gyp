{
  'targets': [
    {
      'target_name': 'action_before_build',
      'type': 'none',
      'hard_dependency': 1,
      'actions': [
        {
          'action_name': 'run_protoc_glyphs',
          'inputs': [
            './proto/glyphs.proto'
          ],
          'outputs': [
            "<(SHARED_INTERMEDIATE_DIR)/glyphs.pb.cc"
          ],
          'action': ['protoc','-Iproto/','--cpp_out=<(SHARED_INTERMEDIATE_DIR)/','./proto/glyphs.proto']
        }
      ]
    },
    {
      'target_name': '<(module_name)',
      'dependencies': [ 'action_before_build' ],
      'sources': [
        'src/node_fontnik.cpp',
        'src/glyphs.cpp',
        'vendor/agg/src/agg_curves.cpp',
        '<(SHARED_INTERMEDIATE_DIR)/glyphs.pb.cc'
      ],
      'include_dirs': [
        './include',
        './vendor/agg/include',
        '<(SHARED_INTERMEDIATE_DIR)/',
        '<!@(pkg-config freetype2 --cflags-only-I | sed s/-I//g)',
        '<!@(pkg-config protobuf --cflags-only-I | sed s/-I//g)',
        "<!(node -e \"require('nan')\")"
      ],
      'libraries': [
        '<!@(pkg-config freetype2 --libs --static)',
        '<!@(pkg-config protobuf --libs --static)'
      ],
      'xcode_settings': {
          'MACOSX_DEPLOYMENT_TARGET': '10.8',
          'OTHER_CPLUSPLUSFLAGS': ['-Wshadow','-std=c++11', '-stdlib=libc++', '-Wno-unused-variable'],
          'GCC_ENABLE_CPP_RTTI': 'YES',
          'GCC_ENABLE_CPP_EXCEPTIONS': 'YES'
      },
      'cflags_cc!': ['-fno-rtti', '-fno-exceptions'],
      'cflags_cc' : ['-std=c++11','-Wshadow'],
      'cflags_c' : ['-std=c99']
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
