{
  'targets': [
    {
      'target_name': 'fontserver',
      'sources': [
        'src/fontserver.cpp',
        'src/font.cpp',
        'src/shaping.cpp',
        'src/sdf_renderer.cpp',
        'src/distmap.c',
        'src/edtaa4func.c'
      ],
      'include_dirs': [ # tried to pass through cflags but failed
        '<!@(pkg-config pangoft2 --cflags-only-I | sed s/-I//g)'
      ],
      'libraries': [
        '<!@(pkg-config pangoft2 --libs)'
      ]
    }
  ]
}
