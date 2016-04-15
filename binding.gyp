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
        '<!@(mason cflags boost ${BOOST_VERSION} | sed s/-I//g)',
        '<!@(mason cflags harfbuzz ${HARFBUZZ_VERSION} | sed s/-I//g)',
        '<!@(mason cflags freetype ${FREETYPE_VERSION} | sed s/-I//g)',
        '<!@(mason cflags protobuf ${PROTOBUF_VERSION} | sed s/-I//g)',
        "<!(node -e \"require('nan')\")"
      ],
      'libraries': [
        '<!@(mason static_libs harfbuzz ${HARFBUZZ_VERSION})',
        '<!@(mason static_libs freetype ${FREETYPE_VERSION})',
        '<!@(mason static_libs protobuf ${PROTOBUF_VERSION})'
      ],
      'conditions': [
        ['OS=="mac"', {
          'xcode_settings': {
            'CLANG_CXX_LIBRARY': 'libc++',
            'CLANG_CXX_LANGUAGE_STANDARD': 'c++1y',
            'GCC_VERSION': 'com.apple.compilers.llvm.clang.1_0',
            'GCC_ENABLE_CPP_EXCEPTIONS': 'YES',
            'GCC_ENABLE_CPP_RTTI': 'YES',
            'OTHER_CPLUSPLUSFLAGS': [
              '-Wall',
              '-Wextra',
              '-Wshadow',
              '-Wno-variadic-macros',
              '-Wno-unused-parameter',
              '-Wno-unused-variable',
              '-Wno-sign-compare',
            ],
            'GCC_WARN_PEDANTIC': 'YES',
            'GCC_WARN_UNINITIALIZED_AUTOS': 'YES_AGGRESSIVE',
            'MACOSX_DEPLOYMENT_TARGET': '10.9',
          },
        }, {
          'cflags_cc': [
            '-std=c++14',
            '-Wall',
            '-Wextra',
            '-Wno-variadic-macros',
            '-Wno-unused-parameter',
            '-Wno-unused-variable',
            '-Wno-sign-compare',
            '-frtti',
            '-fexceptions',
          ],
        }],
      ],
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
