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
        '<(DEPTH)/third_party/icu/icu.gyp:icui18n',
        '<(DEPTH)/third_party/icu/icu.gyp:icuuc',
      ],
      'sources': [
        '../third_party/chromium/src/build/build_config.h',
        '../third_party/chromium/src/base/third_party/dmg_fp/dmg_fp.h',
        '../third_party/chromium/src/base/third_party/dmg_fp/dtoa.cc',
        '../third_party/chromium/src/base/third_party/dmg_fp/g_fmt.cc',
        '../third_party/chromium/src/base/atomicops_internals_x86_gcc.cc',
        '../third_party/chromium/src/base/at_exit.cc',
        '../third_party/chromium/src/base/base_switches.cc',
        '../third_party/chromium/src/base/command_line.cc',
        '../third_party/chromium/src/base/debug_util.cc',
        '../third_party/chromium/src/base/debug_util_mac.cc',
        '../third_party/chromium/src/base/debug_util_posix.cc',
        '../third_party/chromium/src/base/debug_util_win.cc',
        '../third_party/chromium/src/base/dynamic_annotations.cc',
        '../third_party/chromium/src/base/lock.cc',
        '../third_party/chromium/src/base/lock_impl_posix.cc',
        '../third_party/chromium/src/base/lock_impl_win.cc',
        'logging.cc',
        '../third_party/chromium/src/base/platform_thread_mac.mm',
        '../third_party/chromium/src/base/platform_thread_posix.cc',
        '../third_party/chromium/src/base/platform_thread_win.cc',
        '../third_party/chromium/src/base/registry.cc',
        '../third_party/chromium/src/base/setproctitle_linux.c',
        'string16.cc',
        '../third_party/chromium/src/base/string_piece.cc',
        'string_util.cc',
        '../third_party/chromium/src/base/string_util.h',
        '../third_party/chromium/src/base/string_util_icu.cc',
        '../third_party/chromium/src/base/string_util_win.h',
        '../third_party/chromium/src/base/sys_string_conversions.h',
        '../third_party/chromium/src/base/sys_string_conversions_linux.cc',
        '../third_party/chromium/src/base/sys_string_conversions_mac.mm',
        '../third_party/chromium/src/base/sys_string_conversions_win.cc',
        '../third_party/chromium/src/base/win_util.cc',
      ],
      'include_dirs': [
        '../third_party/chromium/src',
      ],
      'direct_dependent_settings': {
        'include_dirs': [
          '../third_party/chromium/src',
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
              '../third_party/chromium/src/base/atomicops_internals_x86_gcc.cc',
            ],
          },
        ],
        [ 'OS != "linux"', {
            'sources!': [
              # Not automatically excluded by the *linux.cc rules.
              '../third_party/chromium/src/base/setproctitle_linux.c',
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
              '../third_party/chromium/src/base/string16.cc',
            ],
          },
          {  # else: OS != "win"
            'sources!': [
              '../third_party/chromium/src/base/registry.cc',
              '../third_party/chromium/src/base/win_util.cc',
            ],
          },
        ],
      ]
    },
    {
      'target_name': 'base_unittests',
      'type': 'executable',
      'sources': [
        '../third_party/chromium/src/base/at_exit_unittest.cc',
        '../third_party/chromium/src/base/atomicops_unittest.cc',
        '../third_party/chromium/src/base/command_line_unittest.cc',
        '../third_party/chromium/src/base/debug_util_unittest.cc',
        '../third_party/chromium/src/base/string_piece_unittest.cc',
        'string_util_unittest.cc',
        '../third_party/chromium/src/base/sys_string_conversions_unittest.cc',
        '../third_party/chromium/src/base/win_util_unittest.cc',
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
            '../third_party/chromium/src/base/win_util_unittest.cc',
          ],
        }],
      ],
    },
  ]
}
