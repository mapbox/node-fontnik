{
  'includes': [ 'common-freetype.gypi' ],
  'target_defaults': {
    'default_configuration': 'Debug',
    'configurations': {
      'Debug': {
        'defines': [ 'DEBUG', '_DEBUG' ],
        'msvs_settings': {
          'VCCLCompilerTool': {
            'RuntimeLibrary': 1, # static debug
          },
        },
      },
      'Release': {
        'defines': [ 'NDEBUG' ],
        'msvs_settings': {
          'VCCLCompilerTool': {
            'RuntimeLibrary': 0, # static release
          },
        },
      }
    },
    'msvs_settings': {
      'VCCLCompilerTool': {
      },
      'VCLibrarianTool': {
      },
      'VCLinkerTool': {
        'GenerateDebugInformation': 'true',
      },
    },
    'conditions': [
      ['OS == "win"', {
        'defines': [
          'WIN32'
        ],
      }]
    ],
  },

  'targets': [
    {
      'target_name': 'freetype_action_before_build',
      'type': 'none',
      'hard_dependency': 1,
      'actions': [
        {
          'action_name': 'unpack_freetype_dep',
          'inputs': [
            './freetype-<@(freetype_version).tar.bz2'
          ],
          'outputs': [
            '<(SHARED_INTERMEDIATE_DIR)/freetype-<@(freetype_version)/src/base/ftbase.c'
          ],
          'action': [ 'tar', '-xjf', './freetype-<@(freetype_version).tar.bz2', '-C', '<(SHARED_INTERMEDIATE_DIR)']
        }
      ],
      'direct_dependent_settings': {
        'include_dirs': [
          '<(SHARED_INTERMEDIATE_DIR)/freetype-<@(freetype_version)',
        ]
      },
    },
    {
      'target_name': 'freetype',
      'type': 'static_library',
      'include_dirs': [
        '../include',
        '<(SHARED_INTERMEDIATE_DIR)/freetype-<@(freetype_version)/include'
      ],
      'dependencies': [
        'freetype_action_before_build'
      ],
      'sources': [
        '<(SHARED_INTERMEDIATE_DIR)/freetype-<@(freetype_version)/src/base/ftsystem.c',
        '<(SHARED_INTERMEDIATE_DIR)/freetype-<@(freetype_version)/src/base/ftinit.c',
        '<(SHARED_INTERMEDIATE_DIR)/freetype-<@(freetype_version)/src/base/ftdebug.c',
        '<(SHARED_INTERMEDIATE_DIR)/freetype-<@(freetype_version)/src/base/ftbase.c',
        '<(SHARED_INTERMEDIATE_DIR)/freetype-<@(freetype_version)/src/base/ftbbox.c',
        '<(SHARED_INTERMEDIATE_DIR)/freetype-<@(freetype_version)/src/base/ftglyph.c',
        '<(SHARED_INTERMEDIATE_DIR)/freetype-<@(freetype_version)/src/base/ftbitmap.c',
        '<(SHARED_INTERMEDIATE_DIR)/freetype-<@(freetype_version)/src/sfnt/sfnt.c',
        '<(SHARED_INTERMEDIATE_DIR)/freetype-<@(freetype_version)/src/truetype/truetype.c',
        '<(SHARED_INTERMEDIATE_DIR)/freetype-<@(freetype_version)/src/smooth/smooth.c',
        '<(SHARED_INTERMEDIATE_DIR)/freetype-<@(freetype_version)/src/autofit/autofit.c'
      ],
      'direct_dependent_settings': {
        'include_dirs': [
          '<(SHARED_INTERMEDIATE_DIR)/freetype-<@(freetype_version)/include'
        ]
      },
      'defines': [
        'FT2_BUILD_LIBRARY'
      ]
    }
  ]
}
