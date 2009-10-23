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
      ],
      'sources': [
        '<(DEPTH)/third_party/chromium/src/build/build_config.h',
        '<(DEPTH)/third_party/chromium/src/base/third_party/dmg_fp/dmg_fp.h',
        '<(DEPTH)/third_party/chromium/src/base/third_party/dmg_fp/dtoa.cc',
        '<(DEPTH)/third_party/chromium/src/base/third_party/dmg_fp/g_fmt.cc',
        '<(DEPTH)/third_party/chromium/src/base/at_exit.cc',
        '<(DEPTH)/third_party/chromium/src/base/debug_util.cc',
        '<(DEPTH)/third_party/chromium/src/base/debug_util_mac.cc',
        '<(DEPTH)/third_party/chromium/src/base/debug_util_posix.cc',
        '<(DEPTH)/third_party/chromium/src/base/debug_util_win.cc',
        '<(DEPTH)/third_party/chromium/src/base/lock.cc',
        '<(DEPTH)/third_party/chromium/src/base/lock_impl_posix.cc',
        '<(DEPTH)/third_party/chromium/src/base/lock_impl_win.cc',
        'logging.cc',
        '<(DEPTH)/third_party/chromium/src/base/platform_thread_mac.mm',
        '<(DEPTH)/third_party/chromium/src/base/platform_thread_posix.cc',
        '<(DEPTH)/third_party/chromium/src/base/platform_thread_win.cc',
        '<(DEPTH)/third_party/chromium/src/base/registry.cc',
        '<(DEPTH)/third_party/chromium/src/base/safe_strerror_posix.cc',
        '<(DEPTH)/third_party/chromium/src/base/setproctitle_linux.c',
        'string16.cc',
        '<(DEPTH)/third_party/chromium/src/base/string_piece.cc',
        'string_util.cc',
        '<(DEPTH)/third_party/chromium/src/base/string_util.h',
        '<(DEPTH)/third_party/chromium/src/base/string_util_win.h',
        '<(DEPTH)/third_party/chromium/src/base/win_util.cc',
      ],
      'include_dirs': [
        '<(DEPTH)/third_party/chromium/src',
      ],
      'direct_dependent_settings': {
        'include_dirs': [
          '<(DEPTH)/third_party/chromium/src',
        ],
      },
      # These warnings are needed for the files in third_party\dmg_fp.
      'msvs_disabled_warnings': [
        4244, 4554, 4018, 4102,
      ],
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
        ],
        [ 'OS != "linux"', {
            'sources!': [
              # Not automatically excluded by the *linux.cc rules.
              '<(DEPTH)/third_party/chromium/src/base/setproctitle_linux.c',
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
              '<(DEPTH)/third_party/chromium/src/base/registry.cc',
              '<(DEPTH)/third_party/chromium/src/base/win_util.cc',
            ],
          },
        ],
      ]
    },
    {
      'target_name': 'base_unittests',
      'type': 'executable',
      'sources': [
        '<(DEPTH)/third_party/chromium/src/base/debug_util_unittest.cc',
        '<(DEPTH)/third_party/chromium/src/base/string_piece_unittest.cc',
        'string_util_unittest.cc',
        '<(DEPTH)/third_party/chromium/src/base/win_util_unittest.cc',
      ],
      'dependencies': [
        'base',
        '<(DEPTH)/testing/gmock.gyp:gmock',
        '<(DEPTH)/testing/gtest.gyp:gtest',
        '<(DEPTH)/testing/gtest.gyp:gtestmain',
      ],
      'include_dirs': [
        '<(DEPTH)',
      ],
      'conditions': [
        ['OS != "win"', {
          'sources!': [
            '<(DEPTH)/third_party/chromium/src/base/win_util_unittest.cc',
          ],
        }],
      ],
    },
  ]
}
