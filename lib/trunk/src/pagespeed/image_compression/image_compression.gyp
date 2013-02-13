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
      'target_name': 'pagespeed_scanline_utils',
      'type': '<(library)',
      'dependencies': [
        '<(DEPTH)/base/base.gyp:base',
        '<(pagespeed_root)/pagespeed/core/core.gyp:pagespeed_core',
      ],
      'sources': [
        'scanline_utils.cc',
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
      'target_name': 'pagespeed_jpeg_utils',
      'type': '<(library)',
      'dependencies': [
        'pagespeed_jpeg_reader',
        '<(DEPTH)/base/base.gyp:base',
        '<(DEPTH)/third_party/libjpeg/libjpeg.gyp:libjpeg',
      ],
      'sources': [
        'jpeg_utils.cc',
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
        '<(DEPTH)/third_party/libjpeg/libjpeg.gyp:libjpeg',
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
      'target_name': 'pagespeed_webp_optimizer',
      'type': '<(library)',
      'dependencies': [
        '<(DEPTH)/base/base.gyp:base',
        '<(DEPTH)/third_party/libwebp/libwebp.gyp:libwebp',
      ],
      'sources': [
        'webp_optimizer.cc',
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
        'pagespeed_scanline_utils',
        '<(DEPTH)/base/base.gyp:base',
        '<(DEPTH)/third_party/libpng/libpng.gyp:libpng',
        '<(DEPTH)/third_party/zlib/zlib.gyp:zlib',
        '<(pagespeed_root)/third_party/giflib/giflib.gyp:dgiflib',
        '<(pagespeed_root)/third_party/optipng/optipng.gyp:opngreduc',
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
      },
      'msvs_disabled_warnings': [
        4996,  # std::string::copy() is deprecated on Windows, but we use it,
               # so we need to disable the warning.
      ],
      'export_dependent_settings': [
        '<(DEPTH)/base/base.gyp:base',
        '<(DEPTH)/third_party/libpng/libpng.gyp:libpng',
        '<(DEPTH)/third_party/zlib/zlib.gyp:zlib',
      ],
    },
    {
      'target_name': 'pagespeed_image_converter',
      'type': '<(library)',
      'dependencies': [
        'pagespeed_jpeg_optimizer',
        'pagespeed_png_optimizer',
        'pagespeed_webp_optimizer',
        '<(DEPTH)/base/base.gyp:base',
      ],
      'sources': [
        'image_converter.cc',
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
      'target_name': 'pagespeed_image_test_util',
      'type': '<(library)',
      'dependencies': [
        'pagespeed_jpeg_reader',
        '<(DEPTH)/base/base.gyp:base',
        '<(DEPTH)/third_party/libjpeg/libjpeg.gyp:libjpeg',
      ],
      'sources': [
        'jpeg_optimizer_test_helper.cc',
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
      'target_name': 'pagespeed_image_resizer',
      'type': '<(library)',
      'dependencies': [
        'pagespeed_scanline_utils',
        '<(DEPTH)/base/base.gyp:base',
      ],
      'sources': [
        'image_resizer.cc',
      ],
      'include_dirs': [
        '<(DEPTH)',
        '<(pagespeed_root)',
      ],
      'direct_dependent_settings': {
        'include_dirs': [
          '<(pagespeed_root)',
        ],
      },
    },
  ],
}
