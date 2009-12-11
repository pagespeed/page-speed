# Copyright 2009 Google Inc.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

{
  'targets': [
    {
      'target_name': 'optipng',
      'type': '<(library)',
      'dependencies': [
        '<(DEPTH)/third_party/zlib/zlib.gyp:zlib',
      ],
      'sources': [
        'src/cbitset.c',
        'src/opngoptim.c',
        'src/opngreduc.c',
        'src/optipng.c',
        'src/osys.c',
        'src/strutil.c',
        'lib/libpng/png.c',
        'lib/libpng/pngerror.c',
        'lib/libpng/pnggccrd.c',
        'lib/libpng/pngget.c',
        'lib/libpng/pngmem.c',
        'lib/libpng/pngpread.c',
        'lib/libpng/pngread.c',
        'lib/libpng/pngrio.c',
        'lib/libpng/pngrtran.c',
        'lib/libpng/pngrutil.c',
        'lib/libpng/pngset.c',
        'lib/libpng/pngtrans.c',
        'lib/libpng/pngvcrd.c',
        'lib/libpng/pngwio.c',
        'lib/libpng/pngwrite.c',
        'lib/libpng/pngwtran.c',
        'lib/libpng/pngwutil.c',
        'lib/pngxtern/gif/gifdump.c',
        'lib/pngxtern/gif/gifread.c',
        'lib/pngxtern/minitiff/minitiff.c',
        'lib/pngxtern/minitiff/tiffread.c',
        'lib/pngxtern/minitiff/tiffwrite.c',
        'lib/pngxtern/pngxio.c',
        'lib/pngxtern/pngxmem.c',
        'lib/pngxtern/pngxrbmp.c',
        'lib/pngxtern/pngxread.c',
        'lib/pngxtern/pngxrgif.c',
        'lib/pngxtern/pngxrjpg.c',
        'lib/pngxtern/pngxrpnm.c',
        'lib/pngxtern/pngxrtif.c',
        'lib/pngxtern/pngxset.c',
        'lib/pngxtern/pngxwrite.c',
        'lib/pngxtern/pnm/pnmin.c',
        'lib/pngxtern/pnm/pnmout.c',
        'lib/pngxtern/pnm/pnmutil.c',
      ],
      'include_dirs': [
        'src',
        'lib/libpng',
        'lib/pngxtern',
      ],
      'direct_dependent_settings': {
        'include_dirs': [
          'src',
        ],
      },
    },
  ],
}
