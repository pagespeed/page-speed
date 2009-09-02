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
            'core/pagespeed_input.proto',
          ],
          'outputs': [
            '<(SHARED_INTERMEDIATE_DIR)/pagespeed/core/pagespeed_input.pb.cc',
            '<(SHARED_INTERMEDIATE_DIR)/pagespeed/core/pagespeed_input.pb.h',
          ],
          'dependencies': [
            '../third_party/protobuf2/protobuf.gyp:protoc',
          ],
          'action': [
            '<(PRODUCT_DIR)/<(EXECUTABLE_PREFIX)protoc<(EXECUTABLE_SUFFIX)',
            '../pagespeed/core/pagespeed_input.proto',
            '--proto_path=..',
            '--cpp_out=<(SHARED_INTERMEDIATE_DIR)',
          ],
        },
      ],
      'sources': [
        '<(SHARED_INTERMEDIATE_DIR)/pagespeed/core/pagespeed_input.pb.cc',
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
            'core/pagespeed_output.proto',
          ],
          'outputs': [
            '<(SHARED_INTERMEDIATE_DIR)/pagespeed/core/pagespeed_output.pb.cc',
            '<(SHARED_INTERMEDIATE_DIR)/pagespeed/core/pagespeed_output.pb.h',
          ],
          'dependencies': [
            '../third_party/protobuf2/protobuf.gyp:protoc',
          ],
          'action': [
            '<(PRODUCT_DIR)/<(EXECUTABLE_PREFIX)protoc<(EXECUTABLE_SUFFIX)',
            '../pagespeed/core/pagespeed_output.proto',
            '--proto_path=..',
            '--cpp_out=<(SHARED_INTERMEDIATE_DIR)',
          ],
        },
      ],
      'sources': [
        '<(SHARED_INTERMEDIATE_DIR)/pagespeed/core/pagespeed_output.pb.cc',
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
      'target_name': 'gzip_details_pb',
      'type': '<(library)',
      'hard_dependency': 1,
      'dependencies': [
          'pagespeed_output_pb',
          '../third_party/protobuf2/protobuf.gyp:protoc',
       ],
      'actions': [
        {
          'action_name': 'my_proto',
          'inputs': [
            '<(PRODUCT_DIR)/<(EXECUTABLE_PREFIX)protoc<(EXECUTABLE_SUFFIX)',
            'rules/gzip_details.proto',
            'core/pagespeed_output.proto',
          ],
          'outputs': [
            '<(SHARED_INTERMEDIATE_DIR)/pagespeed/rules/gzip_details.pb.cc',
            '<(SHARED_INTERMEDIATE_DIR)/pagespeed/rules/gzip_details.pb.h',
          ],
          'dependencies': [
            '../third_party/protobuf2/protobuf.gyp:protoc',
          ],
          'action': [
            '<(PRODUCT_DIR)/<(EXECUTABLE_PREFIX)protoc<(EXECUTABLE_SUFFIX)',
            '../pagespeed/rules/gzip_details.proto',
            '--proto_path=..',
            '--cpp_out=<(SHARED_INTERMEDIATE_DIR)',
          ],
        },
      ],
      'sources': [
        '<(SHARED_INTERMEDIATE_DIR)/pagespeed/rules/gzip_details.pb.cc',
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
      'target_name': 'minimize_dns_details_pb',
      'type': '<(library)',
      'hard_dependency': 1,
      'dependencies': [
          'pagespeed_output_pb',
          '../third_party/protobuf2/protobuf.gyp:protoc',
       ],
      'actions': [
        {
          'action_name': 'my_proto',
          'inputs': [
            '<(PRODUCT_DIR)/<(EXECUTABLE_PREFIX)protoc<(EXECUTABLE_SUFFIX)',
            'rules/minimize_dns_details.proto',
            'core/pagespeed_output.proto',
          ],
          'outputs': [
            '<(SHARED_INTERMEDIATE_DIR)/pagespeed/rules/minimize_dns_details.pb.cc',
            '<(SHARED_INTERMEDIATE_DIR)/pagespeed/rules/minimize_dns_details.pb.h',
          ],
          'dependencies': [
            '../third_party/protobuf2/protobuf.gyp:protoc',
          ],
          'action': [
            '<(PRODUCT_DIR)/<(EXECUTABLE_PREFIX)protoc<(EXECUTABLE_SUFFIX)',
            '../pagespeed/rules/minimize_dns_details.proto',
            '--proto_path=..',
            '--cpp_out=<(SHARED_INTERMEDIATE_DIR)',
          ],
        },
      ],
      'sources': [
        '<(SHARED_INTERMEDIATE_DIR)/pagespeed/rules/minimize_dns_details.pb.cc',
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
      'target_name': 'minimize_resources_details_pb',
      'type': '<(library)',
      'hard_dependency': 1,
      'dependencies': [
          'pagespeed_output_pb',
          '../third_party/protobuf2/protobuf.gyp:protoc',
       ],
      'actions': [
        {
          'action_name': 'my_proto',
          'inputs': [
            '<(PRODUCT_DIR)/<(EXECUTABLE_PREFIX)protoc<(EXECUTABLE_SUFFIX)',
            'rules/minimize_resources_details.proto',
            'core/pagespeed_output.proto',
          ],
          'outputs': [
            '<(SHARED_INTERMEDIATE_DIR)/pagespeed/rules/minimize_resources_details.pb.cc',
            '<(SHARED_INTERMEDIATE_DIR)/pagespeed/rules/minimize_resources_details.pb.h',
          ],
          'dependencies': [
            '../third_party/protobuf2/protobuf.gyp:protoc',
          ],
          'action': [
            '<(PRODUCT_DIR)/<(EXECUTABLE_PREFIX)protoc<(EXECUTABLE_SUFFIX)',
            '../pagespeed/rules/minimize_resources_details.proto',
            '--proto_path=..',
            '--cpp_out=<(SHARED_INTERMEDIATE_DIR)',
          ],
        },
      ],
      'sources': [
        '<(SHARED_INTERMEDIATE_DIR)/pagespeed/rules/minimize_resources_details.pb.cc',
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
        'pagespeed_core',
        'gzip_details_pb',
        'minimize_dns_details_pb',
        'minimize_resources_details_pb',
      ],
      'sources': [
        'rules/gzip_rule.cc',
        'rules/minimize_dns_rule.cc',
        'rules/minimize_resources_rule.cc',
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
        'apps/proto_formatter.cc',
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
      'target_name': 'pagespeed_gzip_rule_test',
      'type': 'executable',
      'dependencies': [
        'pagespeed',
        '../testing/gtest.gyp:gtest',
        '../testing/gtest.gyp:gtestmain',
      ],
      'sources': [
        'rules/gzip_rule_test.cc',
      ],
    },
    {
      'target_name': 'pagespeed_minimize_dns_rule_test',
      'type': 'executable',
      'dependencies': [
        'pagespeed',
        '../testing/gtest.gyp:gtest',
        '../testing/gtest.gyp:gtestmain',
      ],
      'sources': [
        'rules/minimize_dns_rule_test.cc',
      ],
    },
    {
      'target_name': 'pagespeed_minimize_resources_rule_test',
      'type': 'executable',
      'dependencies': [
        'pagespeed',
        '../testing/gtest.gyp:gtest',
        '../testing/gtest.gyp:gtestmain',
      ],
      'sources': [
        'rules/minimize_resources_rule_test.cc',
      ],
    },
    {
      'target_name': 'pagespeed_engine_test',
      'type': 'executable',
      'dependencies': [
        'pagespeed_core',
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
        'apps/proto_formatter_test.cc',
      ],
    },
  ],
}
