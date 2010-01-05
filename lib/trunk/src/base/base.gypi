# Copyright (c) 2009 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

{
  'target_defaults': {
    'variables': {
      'base_target': 0,
      'chromium_root': '<(DEPTH)/third_party/chromium/src',
    },
    'target_conditions': [
      # This part is shared between the targets defined below. Only files and
      # settings relevant for building the Win64 target should be added here.
      # All the rest should be added to the 'base' target below.
      ['base_target==1', {
        'sources': [
        '<(chromium_root)/build/build_config.h',
        '<(chromium_root)/base/third_party/dmg_fp/dmg_fp.h',
        '<(chromium_root)/base/third_party/dmg_fp/dtoa.cc',
        '<(chromium_root)/base/third_party/dmg_fp/g_fmt.cc',
        '<(chromium_root)/base/at_exit.cc',
        '<(chromium_root)/base/debug_util.cc',
        '<(chromium_root)/base/debug_util_mac.cc',
        '<(chromium_root)/base/debug_util_posix.cc',
        '<(chromium_root)/base/debug_util_win.cc',
        '<(chromium_root)/base/lock.cc',
        '<(chromium_root)/base/lock_impl_posix.cc',
        '<(chromium_root)/base/lock_impl_win.cc',
        'logging.cc',
        '<(chromium_root)/base/md5.cc',
        '<(chromium_root)/base/platform_thread_mac.mm',
        '<(chromium_root)/base/platform_thread_posix.cc',
        '<(chromium_root)/base/platform_thread_win.cc',
        '<(chromium_root)/base/registry.cc',
        '<(chromium_root)/base/safe_strerror_posix.cc',
        '<(chromium_root)/base/string_piece.cc',
        'string_util.cc',
        '<(chromium_root)/base/string_util.h',
        '<(chromium_root)/base/string_util_win.h',
        '<(chromium_root)/base/win_util.cc',
        ],
        'include_dirs': [
          '<(chromium_root)',
        ],
        # These warnings are needed for the files in third_party\dmg_fp.
        'msvs_disabled_warnings': [
          4244, 4554, 4018, 4102,
        ],
        'mac_framework_dirs': [
          '$(SDKROOT)/System/Library/Frameworks/ApplicationServices.framework/Frameworks',
        ],
        'conditions': [
          [ 'OS != "linux"', {
              'sources!': [
                # Not automatically excluded by the *linux.cc rules.
                '<(chromium_root)/base/setproctitle_linux.c',
              ],
            },
          ],
          [ 'OS == "win"', {
              'sources!': [
                'string16.cc',
              ],
          },],
        ],
      }],
    ],
  },
  'targets': [
    {
      'target_name': 'base',
      'type': '<(library)',
      'msvs_guid': '1832A374-8A74-4F9E-B536-69A699B3E165',
      'variables': {
        'base_target': 1,
      },
      # TODO(gregoryd): direct_dependent_settings should be shared with the
      #  64-bit target, but it doesn't work due to a bug in gyp
      'direct_dependent_settings': {
        'include_dirs': [
          '<(chromium_root)',
        ],
      },
      # Conditions that are not relevant for Win64 build
      'conditions': [
        [ 'OS == "linux" or OS == "freebsd"', {
          'conditions': [
            [ 'OS == "linux"', {
              'link_settings': {
                'libraries': [
                  # We need rt for clock_gettime().
                  '-lrt',
                ],
              },
            }],
          ],
          'cflags': [
            '-Wno-write-strings',
          ],
        },],
        [ 'OS == "mac"', {
            'link_settings': {
              'libraries': [
                '$(SDKROOT)/System/Library/Frameworks/AppKit.framework',
                '$(SDKROOT)/System/Library/Frameworks/Carbon.framework',
                '$(SDKROOT)/System/Library/Frameworks/CoreFoundation.framework',
                '$(SDKROOT)/System/Library/Frameworks/Foundation.framework',
                '$(SDKROOT)/System/Library/Frameworks/IOKit.framework',
                '$(SDKROOT)/System/Library/Frameworks/Security.framework',
              ],
            },
        },],
        [ 'OS != "win"', {
            'sources!': [
              '<(chromium_root)/base/registry.cc',
              '<(chromium_root)/base/win_util.cc',
            ],
          },
        ],
      ],
      'sources': [
        '<(chromium_root)/base/setproctitle_linux.c',
        'string16.cc',
      ],
    },
  ],
}
