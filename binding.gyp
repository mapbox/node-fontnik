{
  "includes": [ "common.gypi" ],
  "variables": {
    # includes we don't want warnings for.
    # As a variable to make easy to pass to
    # cflags (linux) and xcode (mac)
    "system_includes": [
      "-isystem <(module_root_dir)/<!(node -e \"require('nan')\")",
      "-isystem <(module_root_dir)/mason_packages/.link/include",
      "-isystem <(module_root_dir)/mason_packages/.link/include/freetype2",
      '-isystem <(SHARED_INTERMEDIATE_DIR)'
    ]
  },
  'targets': [
    {
      'target_name': 'action_before_build1',
      'type': 'none',
      'hard_dependency': 1,
      'actions': [
        {
          'action_name': 'install_mason',
          'inputs': ['./install_mason.sh'],
          'outputs': ['./mason_packages'],
          'action': ['./install_mason.sh']
        }
      ]
    },
    {
      'target_name': 'action_before_build2',
      'type': 'none',
      'dependencies': [
        'action_before_build1'
      ],
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
          'action': ['./mason_packages/.link/bin/protoc','-Iproto/','--cpp_out=<(SHARED_INTERMEDIATE_DIR)/','./proto/glyphs.proto']
        }
      ]
    },
    {
      'target_name': '<(module_name)',
      'product_dir': '<(module_path)',
      'dependencies': [
        'action_before_build2'
      ],
      'sources': [
        'src/node_fontnik.cpp',
        'src/glyphs.cpp',
        '<(SHARED_INTERMEDIATE_DIR)/glyphs.pb.cc'
      ],
      "link_settings": {
          "libraries": [
           "-lfreetype",
           "-lprotobuf-lite"
           ],
          "library_dirs": [
            "<(module_root_dir)/mason_packages/.link/lib"
          ]
      },
      'ldflags': [
          '-Wl,-z,now'
      ],
      'cflags_cc': [
          "<@(system_includes)"
      ],
      'include_dirs': [
        './vendor/agg/include',
      ],
      'xcode_settings': {
        'OTHER_LDFLAGS':[
          '-Wl,-bind_at_load'
        ],
        'OTHER_CPLUSPLUSFLAGS': [
            "<@(system_includes)",
        ],
        'GCC_ENABLE_CPP_RTTI': 'YES',
        'GCC_ENABLE_CPP_EXCEPTIONS': 'YES',
        'MACOSX_DEPLOYMENT_TARGET':'10.8',
        'CLANG_CXX_LIBRARY': 'libc++',
        'CLANG_CXX_LANGUAGE_STANDARD':'c++11',
        'GCC_VERSION': 'com.apple.compilers.llvm.clang.1_0'
      }
    }
  ]
}
