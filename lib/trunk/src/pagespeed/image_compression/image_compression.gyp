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
  'variables': {
    'pagespeed_root': '../..',
    'chromium_code': 1,
  },
  'targets': [
    {
      'target_name': 'pagespeed_image_attributes_factory',
      'type': '<(library)',
      'dependencies': [
        'pagespeed_jpeg_reader',
        'pagespeed_png_optimizer',
        '<(DEPTH)/base/base.gyp:base',
        '<(DEPTH)/third_party/libjpeg/libjpeg.gyp:libjpeg',
        '<(DEPTH)/third_party/libpng/libpng.gyp:libpng',
        '<(pagespeed_root)/pagespeed/core/core.gyp:pagespeed_core',
      ],
      'sources': [
        'image_attributes_factory.cc',
      ],
      'include_dirs': [
        '<(pagespeed_root)',
        '<(DEPTH)',
      ],
      'direct_dependent_settings': {
        'include_dirs': [
          '<(pagespeed_root)',
        ],
      },
    },
    {
      'target_name': 'pagespeed_jpeg_reader',
      'type': '<(library)',
      'dependencies': [
        '<(DEPTH)/base/base.gyp:base',
        '<(DEPTH)/third_party/libjpeg/libjpeg.gyp:libjpeg',
      ],
      'sources': [
        'jpeg_reader.cc',
      ],
      'include_dirs': [
        '<(pagespeed_root)',
        '<(DEPTH)',
      ],
      'direct_dependent_settings': {
        'include_dirs': [
          '<(pagespeed_root)',
        ],
      },
    },
    {
      'target_name': 'pagespeed_jpeg_optimizer',
      'type': '<(library)',
      'dependencies': [
        'pagespeed_jpeg_reader',
        '<(DEPTH)/base/base.gyp:base',
        '<(pagespeed_root)/third_party/libjpeg_trans/libjpeg_trans.gyp:libjpeg_trans',
      ],
      'sources': [
        'jpeg_optimizer.cc',
      ],
      'include_dirs': [
        '<(pagespeed_root)',
        '<(DEPTH)',
      ],
      'direct_dependent_settings': {
        'include_dirs': [
          '<(pagespeed_root)',
        ],
      },
    },
    {
      'target_name': 'pagespeed_png_optimizer',
      'type': '<(library)',
      'dependencies': [
        '<(DEPTH)/base/base.gyp:base',
        '<(DEPTH)/third_party/libpng/libpng.gyp:libpng',
        '<(pagespeed_root)/third_party/optipng/optipng.gyp:opngreduc',
        '<(pagespeed_root)/third_party/optipng/optipng.gyp:pngxrgif',
      ],
      'sources': [
        'gif_reader.cc',
        'png_optimizer.cc',
      ],
      'include_dirs': [
        '<(DEPTH)',
        '<(pagespeed_root)',
      ],
      'direct_dependent_settings': {
        'include_dirs': [
          '<(DEPTH)',
          '<(pagespeed_root)',
        ],
        'defines': [
          'PAGESPEED_PNG_OPTIMIZER_GIF_READER'
        ],
      },
      'export_dependent_settings': [
        '<(DEPTH)/base/base.gyp:base',
      ],
      'defines': [
        'PAGESPEED_PNG_OPTIMIZER_GIF_READER'
      ]
    },
  ],
}
