# Copyright (c) 2009 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

{
  'variables': {
    'chromium_libjpeg_root': '<(DEPTH)/third_party/chromium/src/third_party/libjpeg'
  },
  'targets': [
    {
      'target_name': 'libjpeg_trans',
      'type': '<(library)',
      'sources': [
        'jctrans.c',
        'jdtrans.c',
      ],
      'dependencies': [
        '<(chromium_libjpeg_root)/libjpeg.gyp:libjpeg',
      ],
      'export_dependent_settings': [
        '<(chromium_libjpeg_root)/libjpeg.gyp:libjpeg',
      ],
    },
  ],
}
