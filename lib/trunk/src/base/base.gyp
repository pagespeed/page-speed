# Copyright (c) 2009 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# Base was branched from the chromium version to reduce the number of
# dependencies of this package.  Specifically, we would like to avoid
# depending on the chrome directory, which contains the chrome version
# and branding information.
# TODO: push this refactoring to chronium trunk.

{
  'variables': {
    'chromium_code': 1,
  },
  'targets': [
    {
      'target_name': 'base',
      'type': '<(library)',
      'dependencies': [
        '../third_party/icu/icu.gyp:icui18n',
        '../third_party/icu/icu.gyp:icuuc',
      ],
      'sources': [
        '../build/build_config.h',
        'third_party/dmg_fp/dmg_fp.h',
        'third_party/dmg_fp/dtoa.cc',
        'third_party/dmg_fp/g_fmt.cc',
        'atomicops_internals_x86_gcc.cc',
        'at_exit.cc',
        'base_switches.cc',
        'command_line.cc',
        'debug_util.cc',
        'debug_util_mac.cc',
        'debug_util_posix.cc',
        'debug_util_win.cc',
        'dynamic_annotations.cc',
        'lock.cc',
        'lock_impl_posix.cc',
        'lock_impl_win.cc',
        'logging.cc',
        'platform_thread_mac.mm',
        'platform_thread_posix.cc',
        'platform_thread_win.cc',
        'registry.cc',
        'setproctitle_linux.c',
        'string16.cc',
        'string_piece.cc',
        'string_util.cc',
        'string_util.h',
        'string_util_icu.cc',
        'string_util_win.h',
        'sys_string_conversions.h',
        'sys_string_conversions_linux.cc',
        'sys_string_conversions_mac.mm',
        'sys_string_conversions_win.cc',
        'win_util.cc',
      ],
      'include_dirs': [
        '..',
      ],
      'direct_dependent_settings': {
        'include_dirs': [
          '..',
        ],
      },
      'conditions': [
        [ 'OS == "linux"', {
            'sources/': [ ['exclude', '_(mac|win|chromeos)\\.cc$'],
                          ['exclude', '\\.mm?$' ] ],
            'link_settings': {
              'libraries': [
                # We need rt for clock_gettime().
                '-lrt',
              ],
            },
          },
          {  # else: OS != "linux" && OS != "freebsd"
            'sources!': [
              'atomicops_internals_x86_gcc.cc',
            ],
          },
        ],
        [ 'OS != "linux"', {
            'sources!': [
              # Not automatically excluded by the *linux.cc rules.
              'setproctitle_linux.c',
            ],
          },
        ],
        [ 'OS == "mac"', {
            'sources/': [ ['exclude', '_(linux|gtk|win|chromeos)\\.cc$'] ],
            'sources!': [
            ],
            'link_settings': {
              'libraries': [
                '$(SDKROOT)/System/Library/Frameworks/AppKit.framework',
                '$(SDKROOT)/System/Library/Frameworks/Carbon.framework',
                '$(SDKROOT)/System/Library/Frameworks/CoreFoundation.framework',
                '$(SDKROOT)/System/Library/Frameworks/Foundation.framework',
                '$(SDKROOT)/System/Library/Frameworks/Security.framework',
              ],
            },
          }
        ],
        [ 'OS == "win"', {
            'sources/': [ ['exclude', '_(linux|gtk|mac|posix|chromeos)\\.cc$'],
                          ['exclude', '\\.mm?$' ] ],
            'sources!': [
              'string16.cc',
            ],
          },
          {  # else: OS != "win"
            'sources!': [
              'registry.cc',
              'win_util.cc',
            ],
          },
        ],
      ]
    },
    {
      'target_name': 'base_unittests',
      'type': 'executable',
      'sources': [
        'at_exit_unittest.cc',
        'atomicops_unittest.cc',
        'command_line_unittest.cc',
        'debug_util_unittest.cc',
        'string_piece_unittest.cc',
        'string_util_unittest.cc',
        'sys_string_conversions_unittest.cc',
        'win_util_unittest.cc',
      ],
      'dependencies': [
        'base',
        '../testing/gmock.gyp:gmock',
        '../testing/gtest.gyp:gtest',
        '../testing/gtest.gyp:gtestmain',
      ],
      'conditions': [
        ['OS != "win"', {
          'sources!': [
            'win_util_unittest.cc',
          ],
        }],
      ],
    },
  ]
}
