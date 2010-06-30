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
      'target_name': 'pagespeed_bin',
      'type': 'executable',
      'dependencies': [
        '<(DEPTH)/base/base.gyp:base',
        '<(pagespeed_root)/pagespeed/pagespeed.gyp:pagespeed',
        '<(pagespeed_root)/pagespeed/formatters/formatters.gyp:pagespeed_formatters',
        '<(pagespeed_root)/pagespeed/har/har.gyp:pagespeed_har',
        '<(pagespeed_root)/pagespeed/image_compression/image_compression.gyp:pagespeed_image_attributes_factory',
        '<(pagespeed_root)/pagespeed/proto/proto_gen.gyp:pagespeed_input_pb',
        '<(pagespeed_root)/pagespeed/proto/proto.gyp:pagespeed_proto',
      ],
      'sources': [
        'pagespeed.cc',
      ],
    },
    {
      'target_name': 'optimize_image_bin',
      'type': 'executable',
      'dependencies': [
        '<(pagespeed_root)/pagespeed/image_compression/image_compression.gyp:pagespeed_jpeg_optimizer',
        '<(pagespeed_root)/pagespeed/image_compression/image_compression.gyp:pagespeed_png_optimizer',
      ],
      'sources': [
        'optimize_image.cc',
      ],
    },
  ],
}
