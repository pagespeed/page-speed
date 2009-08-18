# Copyright (c) 2009 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# Base was branched from the chronium version to reduce the number of
# dependencies of this package.  Specifically, we would like to avoid
# depending on the chrome directory, which contains the chrome version
# and branding information.
# TODO: push this refactoring to chronium trunk.

{
  'includes': [
    '../build/common.gypi',
  ],
  'targets': [
    {
      'target_name': 'base',
      'type': '<(library)',
      'dependencies': [
        '../third_party/icu38/icu38.gyp:icui18n',
        '../third_party/icu38/icu38.gyp:icuuc',
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
          {  # else: OS != "linux"
            'sources!': [
              'atomicops_internals_x86_gcc.cc',
            ],
          }
        ],
        [ 'OS == "mac"', {
            'sources/': [ ['exclude', '_(linux|win|chromeos)\\.cc$'] ],
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
      ],
      'dependencies': [
        'base',
        '../testing/gmock.gyp:gmock',
        '../testing/gtest.gyp:gtest',
        '../testing/gtest.gyp:gtestmain',
      ],
    },
  ]
}
