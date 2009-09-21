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
  'includes': [
    '../build/common.gypi',
  ],
  'targets': [
    {
      'target_name': 'pagespeed_input_pb',
      'type': '<(library)',
      'hard_dependency': 1,
      'dependencies': [
          '../third_party/protobuf2/protobuf.gyp:protobuf',
          '../third_party/protobuf2/protobuf.gyp:protoc',
       ],
      'actions': [
        {
          'action_name': 'my_proto',
          'inputs': [
            '<(PRODUCT_DIR)/<(EXECUTABLE_PREFIX)protoc<(EXECUTABLE_SUFFIX)',
            'proto/pagespeed_input.proto',
          ],
          'outputs': [
            '<(SHARED_INTERMEDIATE_DIR)/pagespeed/proto/pagespeed_input.pb.cc',
            '<(SHARED_INTERMEDIATE_DIR)/pagespeed/proto/pagespeed_input.pb.h',
          ],
          'dependencies': [
            '../third_party/protobuf2/protobuf.gyp:protoc',
          ],
          'action': [
            '<(PRODUCT_DIR)/<(EXECUTABLE_PREFIX)protoc<(EXECUTABLE_SUFFIX)',
            '../pagespeed/proto/pagespeed_input.proto',
            '--proto_path=..',
            '--cpp_out=<(SHARED_INTERMEDIATE_DIR)',
          ],
        },
      ],
      'sources': [
        '<(SHARED_INTERMEDIATE_DIR)/pagespeed/proto/pagespeed_input.pb.cc',
      ],
      'include_dirs': [
        '<(SHARED_INTERMEDIATE_DIR)',
      ],
      'all_dependent_settings': {
        'include_dirs': [
          '<(SHARED_INTERMEDIATE_DIR)',
        ],
      }
    },
    {
      'target_name': 'pagespeed_output_pb',
      'type': '<(library)',
      'hard_dependency': 1,
      'dependencies': [
          '../third_party/protobuf2/protobuf.gyp:protobuf',
          '../third_party/protobuf2/protobuf.gyp:protoc',
       ],
      'actions': [
        {
          'action_name': 'my_proto',
          'inputs': [
            '<(PRODUCT_DIR)/<(EXECUTABLE_PREFIX)protoc<(EXECUTABLE_SUFFIX)',
            'proto/pagespeed_output.proto',
          ],
          'outputs': [
            '<(SHARED_INTERMEDIATE_DIR)/pagespeed/proto/pagespeed_output.pb.cc',
            '<(SHARED_INTERMEDIATE_DIR)/pagespeed/proto/pagespeed_output.pb.h',
          ],
          'dependencies': [
            '../third_party/protobuf2/protobuf.gyp:protoc',
          ],
          'action': [
            '<(PRODUCT_DIR)/<(EXECUTABLE_PREFIX)protoc<(EXECUTABLE_SUFFIX)',
            '../pagespeed/proto/pagespeed_output.proto',
            '--proto_path=..',
            '--cpp_out=<(SHARED_INTERMEDIATE_DIR)',
          ],
        },
      ],
      'sources': [
        '<(SHARED_INTERMEDIATE_DIR)/pagespeed/proto/pagespeed_output.pb.cc',
      ],
      'include_dirs': [
        '<(SHARED_INTERMEDIATE_DIR)',
      ],
      'all_dependent_settings': {
        'include_dirs': [
          '<(SHARED_INTERMEDIATE_DIR)',
        ],
      }
    },
    {
      'target_name': 'pagespeed_core',
      'type': '<(library)',
      'dependencies': [
        'pagespeed_output_pb',
        '../base/base.gyp:base',
        '../build/temp_gyp/googleurl.gyp:googleurl',
      ],
      'sources': [
        'core/engine.cc',
        'core/formatter.cc',
        'core/pagespeed_input.cc',
        'core/resource.cc',
        'core/rule.cc',
      ],
      'all_dependent_settings': {
        'include_dirs': [
          '..',
        ],
      },
    },
    {
      'target_name': 'pagespeed',
      'type': '<(library)',
      'dependencies': [
        '../third_party/jsmin/jsmin.gyp:jsmin',
        'pagespeed_core',
      ],
      'sources': [
        'rules/combine_external_resources.cc',
        'rules/enable_gzip_compression.cc',
        'rules/minify_javascript.cc',
        'rules/minimize_dns_lookups.cc',
        'rules/minimize_redirects.cc',
        'rules/rule_provider.cc',
      ],
      'include_dirs': [
        '..',
      ],
    },
    {
      'target_name': 'pagespeed_proto',
      'type': '<(library)',
      'dependencies': [
        'pagespeed_input_pb',
        'pagespeed',
      ],
      'sources': [
        'proto/proto_formatter.cc',
        'proto/proto_resource_utils.cc',
      ],
      'include_dirs': [
        '..',
      ],
    },
    {
      'target_name': 'pagespeed_bin',
      'type': 'executable',
      'dependencies': [
        'pagespeed_proto',
      ],
      'sources': [
        'apps/pagespeed.cc',
      ],
    },
    {
      'target_name': 'pagespeed_enable_gzip_compression_test',
      'type': 'executable',
      'dependencies': [
        'pagespeed',
        '../testing/gtest.gyp:gtest',
        '../testing/gtest.gyp:gtestmain',
      ],
      'sources': [
        'rules/enable_gzip_compression_test.cc',
      ],
    },
    {
      'target_name': 'pagespeed_minify_javascript_test',
      'type': 'executable',
      'dependencies': [
        'pagespeed',
        '../testing/gtest.gyp:gtest',
        '../testing/gtest.gyp:gtestmain',
      ],
      'sources': [
        'rules/minify_javascript_test.cc',
      ],
    },
    {
      'target_name': 'pagespeed_combine_external_resources_test',
      'type': 'executable',
      'dependencies': [
        'pagespeed',
        '../testing/gtest.gyp:gtest',
        '../testing/gtest.gyp:gtestmain',
      ],
      'sources': [
        'rules/combine_external_resources_test.cc',
      ],
    },
    {
      'target_name': 'pagespeed_minimize_dns_lookups_test',
      'type': 'executable',
      'dependencies': [
        'pagespeed',
        '../testing/gtest.gyp:gtest',
        '../testing/gtest.gyp:gtestmain',
      ],
      'sources': [
        'rules/minimize_dns_lookups_test.cc',
      ],
    },
    {
      'target_name': 'pagespeed_minimize_redirects_test',
      'type': 'executable',
      'dependencies': [
        'pagespeed',
        '../testing/gtest.gyp:gtest',
        '../testing/gtest.gyp:gtestmain',
      ],
      'sources': [
        'rules/minimize_redirects_test.cc',
      ],
    },
    {
      'target_name': 'pagespeed_engine_test',
      'type': 'executable',
      'dependencies': [
        'pagespeed_core',
        'pagespeed_proto',
        '../testing/gtest.gyp:gtest',
        '../testing/gtest.gyp:gtestmain',
      ],
      'sources': [
        'core/engine_test.cc',
      ],
    },
    {
      'target_name': 'pagespeed_resource_test',
      'type': 'executable',
      'dependencies': [
        'pagespeed_core',
        '../testing/gtest.gyp:gtest',
        '../testing/gtest.gyp:gtestmain',
      ],
      'sources': [
        'core/resource_test.cc',
      ],
    },
    {
      'target_name': 'pagespeed_proto_formatter_test',
      'type': 'executable',
      'dependencies': [
        'pagespeed_proto',
        '../testing/gtest.gyp:gtest',
        '../testing/gtest.gyp:gtestmain',
      ],
      'sources': [
        'proto/proto_formatter_test.cc',
      ],
    },
  ],
}
