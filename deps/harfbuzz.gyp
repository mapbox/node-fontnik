{
  'includes': [ 'common-freetype.gypi', 'common-harfbuzz.gypi' ],
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
      'target_name': 'harfbuzz_action_before_build',
      'type': 'none',
      'hard_dependency': 1,
      'actions': [
        {
          'action_name': 'unpack_harfbuzz_dep',
          'inputs': [
            './harfbuzz-<@(harfbuzz_version).tar.bz2'
          ],
          'outputs': [
            '<(SHARED_INTERMEDIATE_DIR)/harfbuzz-<@(harfbuzz_version)/src/hb-shape.cc'
          ],
          'action': [ 'tar', '-xjf', './harfbuzz-<@(harfbuzz_version).tar.bz2', '-C', '<(SHARED_INTERMEDIATE_DIR)']
        }
      ],
      'direct_dependent_settings': {
        'include_dirs': [
          '../include/harfbuzz',
          '<(SHARED_INTERMEDIATE_DIR)/harfbuzz-<@(harfbuzz_version)/src/hb-ucdn',
          '<(SHARED_INTERMEDIATE_DIR)/harfbuzz-<@(harfbuzz_version)',
        ]
      },
    },
    {
      'target_name': 'harfbuzz',
      'type': 'static_library',
      'include_dirs': [
        '../include',
        '<(SHARED_INTERMEDIATE_DIR)/freetype-<@(freetype_version)/include'
      ],
      'dependencies': [
        'harfbuzz_action_before_build',
        './freetype.gyp:freetype'
      ],
      'defines': [
        'HAVE_CONFIG_H',
        '_THREAD_SAFE'
      ],
      'sources': [
        '<(SHARED_INTERMEDIATE_DIR)/harfbuzz-<@(harfbuzz_version)/src/hb-blob.cc',
        # '<(SHARED_INTERMEDIATE_DIR)/harfbuzz-<@(harfbuzz_version)/src/hb-buffer-serialize.cc',
        '<(SHARED_INTERMEDIATE_DIR)/harfbuzz-<@(harfbuzz_version)/src/hb-buffer.cc',
        '<(SHARED_INTERMEDIATE_DIR)/harfbuzz-<@(harfbuzz_version)/src/hb-common.cc',
        '<(SHARED_INTERMEDIATE_DIR)/harfbuzz-<@(harfbuzz_version)/src/hb-face.cc',
        '<(SHARED_INTERMEDIATE_DIR)/harfbuzz-<@(harfbuzz_version)/src/hb-fallback-shape.cc',
        '<(SHARED_INTERMEDIATE_DIR)/harfbuzz-<@(harfbuzz_version)/src/hb-font.cc',
        '<(SHARED_INTERMEDIATE_DIR)/harfbuzz-<@(harfbuzz_version)/src/hb-ft.cc',
        # '<(SHARED_INTERMEDIATE_DIR)/harfbuzz-<@(harfbuzz_version)/src/hb-icu.cc',
        '<(SHARED_INTERMEDIATE_DIR)/harfbuzz-<@(harfbuzz_version)/src/hb-ot-layout.cc',
        '<(SHARED_INTERMEDIATE_DIR)/harfbuzz-<@(harfbuzz_version)/src/hb-ot-map.cc',
        '<(SHARED_INTERMEDIATE_DIR)/harfbuzz-<@(harfbuzz_version)/src/hb-ot-shape-complex-arabic.cc',
        '<(SHARED_INTERMEDIATE_DIR)/harfbuzz-<@(harfbuzz_version)/src/hb-ot-shape-complex-default.cc',
        '<(SHARED_INTERMEDIATE_DIR)/harfbuzz-<@(harfbuzz_version)/src/hb-ot-shape-complex-indic-table.cc',
        '<(SHARED_INTERMEDIATE_DIR)/harfbuzz-<@(harfbuzz_version)/src/hb-ot-shape-complex-indic.cc',
        '<(SHARED_INTERMEDIATE_DIR)/harfbuzz-<@(harfbuzz_version)/src/hb-ot-shape-complex-myanmar.cc',
        '<(SHARED_INTERMEDIATE_DIR)/harfbuzz-<@(harfbuzz_version)/src/hb-ot-shape-complex-sea.cc',
        '<(SHARED_INTERMEDIATE_DIR)/harfbuzz-<@(harfbuzz_version)/src/hb-ot-shape-complex-thai.cc',
        '<(SHARED_INTERMEDIATE_DIR)/harfbuzz-<@(harfbuzz_version)/src/hb-ot-shape-fallback.cc',
        '<(SHARED_INTERMEDIATE_DIR)/harfbuzz-<@(harfbuzz_version)/src/hb-ot-shape-normalize.cc',
        '<(SHARED_INTERMEDIATE_DIR)/harfbuzz-<@(harfbuzz_version)/src/hb-ot-shape.cc',
        '<(SHARED_INTERMEDIATE_DIR)/harfbuzz-<@(harfbuzz_version)/src/hb-ot-tag.cc',
        # '<(SHARED_INTERMEDIATE_DIR)/harfbuzz-<@(harfbuzz_version)/src/hb-set.cc',
        '<(SHARED_INTERMEDIATE_DIR)/harfbuzz-<@(harfbuzz_version)/src/hb-shape-plan.cc',
        '<(SHARED_INTERMEDIATE_DIR)/harfbuzz-<@(harfbuzz_version)/src/hb-shape.cc',
        '<(SHARED_INTERMEDIATE_DIR)/harfbuzz-<@(harfbuzz_version)/src/hb-shaper.cc',
        # '<(SHARED_INTERMEDIATE_DIR)/harfbuzz-<@(harfbuzz_version)/src/hb-tt-font.cc',
        '<(SHARED_INTERMEDIATE_DIR)/harfbuzz-<@(harfbuzz_version)/src/hb-ucdn.cc',
        '<(SHARED_INTERMEDIATE_DIR)/harfbuzz-<@(harfbuzz_version)/src/hb-ucdn/ucdn.c',
        '<(SHARED_INTERMEDIATE_DIR)/harfbuzz-<@(harfbuzz_version)/src/hb-unicode.cc',
        # '<(SHARED_INTERMEDIATE_DIR)/harfbuzz-<@(harfbuzz_version)/src/hb-uniscribe.cc',
        # '<(SHARED_INTERMEDIATE_DIR)/harfbuzz-<@(harfbuzz_version)/src/hb-warning.cc'
      ]
    }
  ]
}
