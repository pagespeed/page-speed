# Copyright (c) 2009 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

{
  'variables': {
    'conditions': [
      [ 'os_posix == 1 and OS != "mac"', {
        # Link to system .so since we already use it due to GTK.
        'use_system_libjpeg%': 1,
      }, {  # os_posix != 1 or OS == "mac"
        'use_system_libjpeg%': 0,
      }],
    ],
  },
  'conditions': [
    ['use_system_libjpeg==0', {
      'targets': [
        {
          'target_name': 'libjpeg',
          'type': 'static_library',
          'sources': [
            'jconfig.h',
            'src/jaricom.c',
            'src/jcapimin.c',
            'src/jcapistd.c',
            'src/jcarith.c',
            'src/jccoefct.c',
            'src/jccolor.c',
            'src/jcdctmgr.c',
            'src/jchuff.c',
            'src/jcinit.c',
            'src/jcmainct.c',
            'src/jcmarker.c',
            'src/jcmaster.c',
            'src/jcomapi.c',
            'src/jcparam.c',
            'src/jcprepct.c',
            'src/jcsample.c',
            'src/jctrans.c',
            'src/jdapimin.c',
            'src/jdapistd.c',
            'src/jdarith.c',
            'src/jdatadst.c',
            'src/jdatasrc.c',
            'src/jdcoefct.c',
            'src/jdcolor.c',
            'src/jdct.h',
            'src/jddctmgr.c',
            'src/jdhuff.c',
            'src/jdinput.c',
            'src/jdmainct.c',
            'src/jdmarker.c',
            'src/jdmaster.c',
            'src/jdmerge.c',
            'src/jdpostct.c',
            'src/jdsample.c',
            'src/jdtrans.c',
            'src/jerror.c',
            'src/jerror.h',
            'src/jfdctflt.c',
            'src/jfdctfst.c',
            'src/jfdctint.c',
            'src/jidctflt.c',
            'src/jidctfst.c',
            'src/jidctint.c',
            'src/jinclude.h',
            'src/jmemmgr.c',
            'src/jmemnobs.c',
            'src/jmemsys.h',
            'src/jmorecfg.h',
            'src/jpegint.h',
            'src/jpeglib.h',
            'src/jquant1.c',
            'src/jquant2.c',
            'src/jutils.c',
            'src/jversion.h',
          ],
          'include_dirs': [
            '<(DEPTH)',
            '.',
            'src',
          ],
          'direct_dependent_settings': {
            'include_dirs': [
              '<(DEPTH)',
              '.',
              'src',
            ],
          },
          'conditions': [
            ['OS!="win"', {'product_name': 'jpeg'}],
          ],
        },
      ],
    }, {
      'targets': [
        {
          'target_name': 'libjpeg',
          'type': 'settings',
          'direct_dependent_settings': {
            'defines': [
              'USE_SYSTEM_LIBJPEG',
            ],
          },
          'link_settings': {
            'libraries': [
              '-ljpeg',
            ],
          },
        }
      ],
    }],
  ],
}

# Local Variables:
# tab-width:2
# indent-tabs-mode:nil
# End:
# vim: set expandtab tabstop=2 shiftwidth=2:
