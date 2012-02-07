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
      # We include various proto libraries in
      # export_dependent_settings however their hard_dependency does
      # not get propagated. Thus we temporarily mark this target as a
      # hard_dependency. See
      # http://code.google.com/p/gyp/issues/detail?id=248 for the bug
      # that tracks this issue.
      'hard_dependency': 1,
      'dependencies': [
        '<(DEPTH)/base/base.gyp:base',
        '<(DEPTH)/testing/gtest.gyp:gtest',
        '<(DEPTH)/third_party/gflags/gflags.gyp:gflags',
        '<(pagespeed_root)/pagespeed/core/core.gyp:pagespeed_core',
        '<(pagespeed_root)/pagespeed/formatters/formatters.gyp:pagespeed_formatters',
        '<(pagespeed_root)/pagespeed/proto/proto_gen.gyp:pagespeed_output_pb',
        '<(pagespeed_root)/pagespeed/proto/proto_gen.gyp:pagespeed_proto_formatted_results_converter',
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
      'export_dependent_settings': [
        '<(DEPTH)/testing/gtest.gyp:gtest',
        '<(pagespeed_root)/pagespeed/proto/proto_gen.gyp:pagespeed_output_pb',
      ],
    },
    {
      'target_name': 'pagespeed_test_main',
      'type': '<(library)',
      'dependencies': [
        '<(DEPTH)/testing/gtest.gyp:gtest',
        '<(DEPTH)/third_party/gflags/gflags.gyp:gflags',
        '<(DEPTH)/<(protobuf_gyp_path):protobuf_lite',
        '<(pagespeed_root)/pagespeed/core/init.gyp:pagespeed_init',
      ],
      'sources': [
        'pagespeed_test_main.cc',
      ],
      'include_dirs': [
        '<(DEPTH)',
        '<(pagespeed_root)',
      ],
    },
  ],
}
