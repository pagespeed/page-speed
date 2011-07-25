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
      'target_name': 'pagespeed_har',
      'type': '<(library)',
      'dependencies': [
        '<(DEPTH)/base/base.gyp:base',
        '<(DEPTH)/third_party/modp_b64/modp_b64.gyp:modp_b64',
        '<(pagespeed_root)/pagespeed/core/core.gyp:pagespeed_core',
      ],
      'sources': [
        'http_archive.cc',
      ],
      'include_dirs': [
        '<(DEPTH)',
        '<(pagespeed_root)',
      ],
      'conditions': [
        ['OS=="win"', {
          'defines': [
            # Suppress a warning about sscanf on Windows.
            '_CRT_SECURE_NO_WARNINGS',
          ],
        }],
      ],
    },
  ],
}
