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
    'chromium_root': '<(DEPTH)/third_party/chromium/src',
  },
  'includes': [
    'base.gypi',
  ],
  'targets': [
    {
      'target_name': 'base_unittests',
      'type': 'executable',
      'msvs_guid': '27A30967-4BBA-48D1-8522-CDE95F7B1CEC',
      'sources': [
        '<(chromium_root)/base/debug_util_unittest.cc',
        '<(chromium_root)/base/string_piece_unittest.cc',
        'string_util_unittest.cc',
        '<(chromium_root)/base/win_util_unittest.cc',
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
            '<(chromium_root)/base/win_util_unittest.cc',
          ],
        }],
      ],
    },
  ],
}
