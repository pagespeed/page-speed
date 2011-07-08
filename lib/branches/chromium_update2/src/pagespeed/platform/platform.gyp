# Copyright 2010 Google Inc.
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
      # The xcode gyp generator complains if we have an empty targets
      # list, so we define this one target to make it happy.
      'target_name': 'pagespeed_platform_dummy',
      'type': 'none',
    }
  ],
  'conditions': [
    ['OS=="win"', {
      'targets': [
        {
          'target_name': 'pagesped_platform_ie_dom',
          'type': '<(library)',
          'dependencies': [
            '<(pagespeed_root)/pagespeed/core/core.gyp:pagespeed_core',
          ],
          'sources': [
            'ie/ie_dom.cc',
          ],
        }
      ]
    }],
  ],
}
