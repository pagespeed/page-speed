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
      'target_name': 'pagespeed_core',
      'type': '<(library)',
      'dependencies': [
        '<(DEPTH)/base/base.gyp:base',
        '<(DEPTH)/build/temp_gyp/googleurl.gyp:googleurl',
        '<(pagespeed_root)/pagespeed/proto/proto_gen.gyp:pagespeed_output_pb',
        '<(pagespeed_root)/pagespeed/proto/proto_gen.gyp:timeline_pb',
      ],
      'sources': [
        'directive_enumerator.cc',
        'dom.cc',
        'engine.cc',
        'file_util.cc',
        'formatter.cc',
        'image_attributes.cc',
        'pagespeed_input.cc',
        'pagespeed_input_util.cc',
        'pagespeed_version.cc',
        'resource.cc',
        'resource_filter.cc',
        'resource_util.cc',
        'result_provider.cc',
        'rule.cc',
        'rule_input.cc',
        'string_util.cc',
        'timeline.cc',
        'uri_util.cc',
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
        '<(DEPTH)/base/base.gyp:base',
        '<(DEPTH)/build/temp_gyp/googleurl.gyp:googleurl',
      ]
    },
  ],
}
