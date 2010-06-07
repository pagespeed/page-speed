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
    'protoc_out_dir': '<(SHARED_INTERMEDIATE_DIR)/protoc_out',
  },
  'targets': [
    {
      'target_name': 'pagespeed_genproto',
      'type': 'none',
      'sources': [
        'pagespeed_input.proto',
        'pagespeed_output.proto',
      ],
      'rules': [
        {
          'rule_name': 'genproto',
          'extension': 'proto',
          'inputs': [
            '<(PRODUCT_DIR)/<(EXECUTABLE_PREFIX)protoc<(EXECUTABLE_SUFFIX)',
          ],
          'variables': {
            # The protoc compiler requires a proto_path argument with the
            # directory containing the .proto file.
            # There's no generator variable that corresponds to this, so fake it.
            'rule_input_relpath': '.',
          },
          'outputs': [
            '<(protoc_out_dir)/pagespeed/proto/<(rule_input_relpath)/<(RULE_INPUT_ROOT).pb.h',
            '<(protoc_out_dir)/pagespeed/proto/<(rule_input_relpath)/<(RULE_INPUT_ROOT).pb.cc',
          ],
          'action': [
            '<(PRODUCT_DIR)/<(EXECUTABLE_PREFIX)protoc<(EXECUTABLE_SUFFIX)',
            '--proto_path=./<(rule_input_relpath)',
            './<(rule_input_relpath)/<(RULE_INPUT_ROOT)<(RULE_INPUT_EXT)',
            '--cpp_out=<(protoc_out_dir)/pagespeed/proto/<(rule_input_relpath)',
          ],
          'message': 'Generating C++ code from <(RULE_INPUT_PATH)',
        },
      ],
      'dependencies': [
        '<(DEPTH)/third_party/protobuf2/protobuf.gyp:protobuf_lite',
        '<(DEPTH)/third_party/protobuf2/protobuf.gyp:protoc#host',
      ],
      'direct_dependent_settings': {
        'include_dirs': [
          '<(protoc_out_dir)',
        ]
      },
      'export_dependent_settings': [
        '<(DEPTH)/third_party/protobuf2/protobuf.gyp:protobuf_lite',
      ],
    },
    {
      'target_name': 'pagespeed_input_pb',
      'type': '<(library)',
      'hard_dependency': 1,
      'dependencies': [
        'pagespeed_genproto',
        '<(DEPTH)/third_party/protobuf2/protobuf.gyp:protobuf_lite',
       ],
      'sources': [
        '<(protoc_out_dir)/pagespeed/proto/pagespeed_input.pb.cc',
      ],
      'export_dependent_settings': [
        'pagespeed_genproto',
        '<(DEPTH)/third_party/protobuf2/protobuf.gyp:protobuf_lite',
      ]
    },
    {
      'target_name': 'pagespeed_output_pb',
      'type': '<(library)',
      'hard_dependency': 1,
      'dependencies': [
        'pagespeed_genproto',
        '<(DEPTH)/third_party/protobuf2/protobuf.gyp:protobuf_lite',
       ],
      'sources': [
        '<(protoc_out_dir)/pagespeed/proto/pagespeed_output.pb.cc',
      ],
      'export_dependent_settings': [
        'pagespeed_genproto',
        '<(DEPTH)/third_party/protobuf2/protobuf.gyp:protobuf_lite',
      ]
    },
  ],
}
