{
  'targets': [
    {
      'target_name': 'action_before_build',
      'type': 'none',
      'hard_dependency': 1,
      'actions': [
        {
          'action_name': 'run_protoc_metrics',
          'inputs': [
            './proto/metrics.proto'
          ],
          'outputs': [
            "./src/metrics.pb.cc"
          ],
          'action': ['protoc','-Iproto/','--cpp_out=./src/','./proto/metrics.proto']
        },
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
        'src/fontserver.cpp',
        'src/ft_font.cpp',
        'src/pango_font.cpp',
        'src/pango_shaper.cpp',
        'src/tile.cpp',
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
        '<!@(pkg-config freetype2 --cflags-only-I | sed s/-I//g)',
        '<!@(pkg-config harfbuzz --cflags-only-I | sed s/-I//g)',
        '<!@(pkg-config protobuf --cflags-only-I | sed s/-I//g)',
      ],
      'libraries': [
        '<!@(pkg-config pangoft2 --libs)',
        '<!@(pkg-config freetype2 --libs)',
        '<!@(pkg-config harfbuzz --libs)',
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
