{
  'target_defaults': {
    'default_configuration': 'Release',
    'cflags_cc' : [
      '-std=c++14',
      # mason packages are built/linked with the CXX11_ABI=0 (currently)
      # so we need to link this way too. This allows source
      # compiling carmen-cache on any ubuntu version, even latest
      # where the CXX11_ABI default has flipped to 1
      '-D_GLIBCXX_USE_CXX11_ABI=0'
    ],
    'cflags_cc!': ['-std=gnu++0x','-fno-rtti', '-fno-exceptions'],
    'configurations': {
      'Debug': {
        'defines!': [
          'NDEBUG'
        ],
        'cflags_cc!': [
          '-O3',
          '-Os',
          '-DNDEBUG'
        ],
        'xcode_settings': {
          'OTHER_CPLUSPLUSFLAGS!': [
            '-O3',
            '-Os',
            '-DDEBUG'
          ],
          'GCC_OPTIMIZATION_LEVEL': '0',
          'GCC_GENERATE_DEBUGGING_SYMBOLS': 'YES'
        }
      },
      'Release': {
        'defines': [
          'NDEBUG'
        ],
        'xcode_settings': {
          'OTHER_CPLUSPLUSFLAGS!': [
            '-Os',
            '-O2'
          ],
          'GCC_OPTIMIZATION_LEVEL': '3',
          'GCC_GENERATE_DEBUGGING_SYMBOLS': 'NO',
          'DEAD_CODE_STRIPPING': 'YES',
          'GCC_INLINES_ARE_PRIVATE_EXTERN': 'YES'
        }
      }
    }
  }
}