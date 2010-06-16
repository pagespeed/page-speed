# Copyright (c) 2009 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

{
  'variables': {
    'chromium_libjpeg_root': '<(DEPTH)/third_party/libjpeg',
    'conditions': [
      [ 'OS=="linux" or OS=="freebsd" or OS=="openbsd"', {
        # Link to system .so since we already use it due to GTK.
        'use_system_libjpeg%': 1,
      }, {  # OS!="linux" and OS!="freebsd" and OS!="openbsd"
        'use_system_libjpeg%': 0,
      }],
    ],
  },
  'conditions': [
    ['use_system_libjpeg==0', {
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
    }, {
      'targets': [
        {
          'target_name': 'libjpeg_trans',
          'type': 'settings',
          'dependencies': [
            '<(chromium_libjpeg_root)/libjpeg.gyp:libjpeg',
          ],
          'export_dependent_settings': [
            '<(chromium_libjpeg_root)/libjpeg.gyp:libjpeg',
          ],
        }
      ],
    }],
  ],
}
