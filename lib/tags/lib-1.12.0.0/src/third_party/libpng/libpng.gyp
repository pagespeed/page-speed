# Copyright (c) 2009 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

{
  'variables': {
    'conditions': [
      [ 'os_posix == 1 and OS != "mac"', {
        # Link to system .so since we already use it due to GTK.
        'use_system_libpng%': 1,
      }, {  # os_posix != 1 or OS == "mac"
        'use_system_libpng%': 0,
      }],
    ],
  },
  'conditions': [
    ['use_system_libpng==0', {
      'targets': [
        {
          'target_name': 'libpng',
          'type': '<(component)',
          'dependencies': [
            '../zlib/zlib.gyp:zlib',
          ],
          'sources': [
            'png.c',
            'png.h',
            'pngconf.h',
            'pngdebug.h',
            'pngerror.c',
            'pngget.c',
            'pnginfo.h',
            'pngmem.c',
            'pngpread.c',
            'pngpriv.h',
            'pngread.c',
            'pngrio.c',
            'pngrtran.c',
            'pngrutil.c',
            'pngset.c',
            'pngstruct.h',
            'pngtrans.c',
            'pngwio.c',
            'pngwrite.c',
            'pngwtran.c',
            'pngwutil.c',
          ],
          'direct_dependent_settings': {
            'include_dirs': [
              '.',
            ],
            'defines': [
              'PNG_FREE_ME_SUPPORTED',
            ],
          },
          'export_dependent_settings': [
            '../zlib/zlib.gyp:zlib',
          ],
          'conditions': [
            ['OS!="win"', {'product_name': 'png'}],
            ['OS=="win" and component=="shared_library"', {
              'defines': [
                'PNG_BUILD_DLL',
                'PNG_NO_MODULEDEF',
              ],
              'direct_dependent_settings': {
                'defines': [
                  'PNG_USE_DLL',
                ],
              },
            }],
          ],
        },
      ]
    }, {
      'conditions': [
        ['sysroot!=""', {
          'variables': {
            'pkg-config': '../../build/linux/pkg-config-wrapper "<(sysroot)"',
          },
        }, {
          'variables': {
            'pkg-config': 'pkg-config'
          },
        }],
      ],
      'targets': [
        {
          'target_name': 'libpng',
          'type': 'settings',
          'dependencies': [
            '../zlib/zlib.gyp:zlib',
          ],
          'direct_dependent_settings': {
            'cflags': [
              '<!@(<(pkg-config) --cflags libpng)',
            ],
            'defines': [
              'USE_SYSTEM_LIBPNG',
            ],
          },
          'link_settings': {
            'ldflags': [
              '<!@(<(pkg-config) --libs-only-L --libs-only-other libpng)',
            ],
            'libraries': [
              '<!@(<(pkg-config) --libs-only-l libpng)',
            ],
          },
        },
      ],
    }],
  ],
}

# Local Variables:
# tab-width:2
# indent-tabs-mode:nil
# End:
# vim: set expandtab tabstop=2 shiftwidth=2:
