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
            'pnglibconf.h',
            'src/png.c',
            'src/png.h',
            'src/pngconf.h',
            'src/pngdebug.h',
            'src/pngerror.c',
            'src/pngget.c',
            'src/pnginfo.h',
            'src/pngmem.c',
            'src/pngpread.c',
            'src/pngpriv.h',
            'src/pngread.c',
            'src/pngrio.c',
            'src/pngrtran.c',
            'src/pngrutil.c',
            'src/pngset.c',
            'src/pngstruct.h',
            'src/pngtrans.c',
            'src/pngwio.c',
            'src/pngwrite.c',
            'src/pngwtran.c',
            'src/pngwutil.c',
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
