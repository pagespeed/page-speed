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
      'target_name': 'pagespeed_filters',
      'type': '<(library)',
      'dependencies': [
        '<(pagespeed_root)/pagespeed/core/core.gyp:pagespeed_core',
        '<(pagespeed_root)/pagespeed/proto/proto_gen.gyp:pagespeed_output_pb',
        '<(pagespeed_root)/pagespeed/util/util.gyp:pagespeed_util',
        '<(pagespeed_root)/third_party/adblockrules/adblockrules.gyp:adblockrules',
      ],
      'sources': [
        'ad_filter.cc',
        'landing_page_redirection_filter.cc',
        'protocol_filter.cc',
        'response_byte_result_filter.cc',
        'tracker_filter.cc',
        'url_regex_filter.cc',
      ],
      'include_dirs': [
        '<(pagespeed_root)',
      ],
      'direct_dependent_settings': {
        'include_dirs': [
          '<(pagespeed_root)',
        ],
      },
      'export_dependent_settings': [
        '<(pagespeed_root)/third_party/adblockrules/adblockrules.gyp:adblockrules',
      ]
    },
  ],
}
