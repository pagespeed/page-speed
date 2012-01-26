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
      'target_name': 'pagespeed_testing',
      'type': '<(library)',
      'dependencies': [
        '<(DEPTH)/base/base.gyp:base',
        '<(DEPTH)/testing/gtest.gyp:gtest',
        '<(pagespeed_root)/pagespeed/core/core.gyp:pagespeed_core',
        '<(pagespeed_root)/pagespeed/formatters/formatters.gyp:pagespeed_formatters',
        '<(pagespeed_root)/pagespeed/proto/proto_gen.gyp:pagespeed_output_pb',
        '<(pagespeed_root)/pagespeed/proto/proto_gen.gyp:pagespeed_proto_formatted_results_converter',
        '<(pagespeed_root)/third_party/gflags/gflags.gyp:gflags',
      ],
      'sources': [
        'fake_dom.cc',
        'formatted_results_test_converter.cc',
        'instrumentation_data_builder.cc',
        'pagespeed_test.cc',
      ],
      'include_dirs': [
        '<(pagespeed_root)',
      ],
      'direct_dependent_settings': {
        'include_dirs': [
          '<(pagespeed_root)',
        ],
      },
    },
    {
      'target_name': 'pagespeed_test_main',
      'type': '<(library)',
      'dependencies': [
        '<(DEPTH)/testing/gtest.gyp:gtest',
        '<(DEPTH)/<(protobuf_gyp_path):protobuf_lite',
        '<(pagespeed_root)/pagespeed/core/init.gyp:pagespeed_init',
        '<(pagespeed_root)/third_party/gflags/gflags.gyp:gflags',
      ],
      'sources': [
        'pagespeed_test_main.cc',
      ],
      'include_dirs': [
        '<(pagespeed_root)',
      ],
    },
  ],
}
