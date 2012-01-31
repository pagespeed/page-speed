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
    'protoc_out_dir': '<(SHARED_INTERMEDIATE_DIR)/protoc_out',
    'java_objroot': '<(DEPTH)/out/java',
    'pagespeed_proto_java_objroot': '<(java_objroot)/classes/pagespeed_proto',
    'pagespeed_proto_java_srcroot': '<(protoc_out_dir)/pagespeed/proto',
  },
  'targets': [
    {
      'target_name': 'pagespeed_genproto',
      'type': 'none',
      'sources': [
        'pagespeed_input.proto',
        'pagespeed_output.proto',
        'pagespeed_proto_formatter.proto',
        'resource.proto',
        'timeline.proto',
      ],
      'rules': [
        {
          'rule_name': 'genproto',
          'extension': 'proto',
          'inputs': [
            '<(PRODUCT_DIR)/<(EXECUTABLE_PREFIX)protoc<(EXECUTABLE_SUFFIX)',
          ],
          'outputs': [
            '<(protoc_out_dir)/pagespeed/proto/<(RULE_INPUT_ROOT).pb.h',
            '<(protoc_out_dir)/pagespeed/proto/<(RULE_INPUT_ROOT).pb.cc',
          ],
          'action': [
            '<(PRODUCT_DIR)/<(EXECUTABLE_PREFIX)protoc<(EXECUTABLE_SUFFIX)',
            '--proto_path=',
            './<(RULE_INPUT_ROOT)<(RULE_INPUT_EXT)',
            '--cpp_out=<(protoc_out_dir)/pagespeed/proto',
            '--java_out=<(pagespeed_proto_java_srcroot)',
          ],
          'message': 'Generating C++ code from <(RULE_INPUT_PATH)',
        },
      ],
      'dependencies': [
        '<(DEPTH)/<(protobuf_gyp_path):protobuf_lite',
        '<(DEPTH)/<(protobuf_gyp_path):protoc#host',
      ],
      'direct_dependent_settings': {
        'include_dirs': [
          '<(protoc_out_dir)',
        ]
      },
      'export_dependent_settings': [
        '<(DEPTH)/<(protobuf_gyp_path):protobuf_lite',
      ],
    },
    {
      'target_name': 'pagespeed_input_pb',
      'type': '<(library)',
      'hard_dependency': 1,
      'dependencies': [
        'pagespeed_genproto',
        '<(DEPTH)/<(protobuf_gyp_path):protobuf_lite',
       ],
      'sources': [
        '<(protoc_out_dir)/pagespeed/proto/pagespeed_input.pb.cc',
      ],
      'export_dependent_settings': [
        'pagespeed_genproto',
        '<(DEPTH)/<(protobuf_gyp_path):protobuf_lite',
      ]
    },
    {
      'target_name': 'pagespeed_output_pb',
      'type': '<(library)',
      'hard_dependency': 1,
      'dependencies': [
        'pagespeed_genproto',
        '<(DEPTH)/<(protobuf_gyp_path):protobuf_lite',
       ],
      'sources': [
        '<(protoc_out_dir)/pagespeed/proto/pagespeed_output.pb.cc',
        '<(protoc_out_dir)/pagespeed/proto/pagespeed_proto_formatter.pb.cc',
      ],
      'export_dependent_settings': [
        'pagespeed_genproto',
        '<(DEPTH)/<(protobuf_gyp_path):protobuf_lite',
      ]
    },
    {
      'target_name': 'timeline_pb',
      'type': '<(library)',
      'hard_dependency': 1,
      'dependencies': [
        'pagespeed_genproto',
        '<(DEPTH)/<(protobuf_gyp_path):protobuf_lite',
       ],
      'sources': [
        '<(protoc_out_dir)/pagespeed/proto/timeline.pb.cc',
      ],
      'export_dependent_settings': [
        'pagespeed_genproto',
        '<(DEPTH)/<(protobuf_gyp_path):protobuf_lite',
      ]
    },
    {
      'target_name': 'pagespeed_resource_pb',
      'type': '<(library)',
      'hard_dependency': 1,
      'dependencies': [
        'pagespeed_genproto',
        '<(DEPTH)/<(protobuf_gyp_path):protobuf_lite',
       ],
      'sources': [
        '<(protoc_out_dir)/pagespeed/proto/resource.pb.cc',
      ],
      'export_dependent_settings': [
        'pagespeed_genproto',
        '<(DEPTH)/<(protobuf_gyp_path):protobuf_lite',
      ]
    },
    {
      'target_name': 'pagespeed_proto_results_converter',
      'type': '<(library)',
      'dependencies': [
        '<(DEPTH)/base/base.gyp:base',
        '<(pagespeed_root)/pagespeed/proto/proto_gen.gyp:pagespeed_output_pb',
      ],
      'sources': [
        'results_to_json_converter.cc',
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
      ]
    },
    {
      'target_name': 'pagespeed_proto_formatted_results_converter',
      'type': '<(library)',
      'dependencies': [
        '<(DEPTH)/base/base.gyp:base',
        '<(pagespeed_root)/pagespeed/proto/proto_gen.gyp:pagespeed_output_pb',
      ],
      'sources': [
        'formatted_results_to_json_converter.cc',
        'formatted_results_to_text_converter.cc',
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
      ]
    },
    {
      'target_name': 'pagespeed_proto_java_javac',
      'suppress_wildcard': 1,
      'type': 'none',
      'dependencies': [
        'pagespeed_genproto',
        '<(pagespeed_root)/build/temp_gyp/protobuf_java.gyp:protobuf_java_jar',
      ],
      'actions': [
        {
          'action_name': 'javac',
          'inputs': [
            # gyp gets confused when we declare the .java files in the
            # outputs section of the gen rule, so we don't. Since the
            # .pb.h and .java files are generated in the same s tep,
            # we claim to depend on the .pb.h file so gyp can property
            # infer dependencies.
            '<(protoc_out_dir)/pagespeed/proto/pagespeed_output.pb.h',
          ],
          'outputs': [
            '<(pagespeed_proto_java_objroot)/com/googlecode/page_speed/PagespeedInput.class',
            '<(pagespeed_proto_java_objroot)/com/googlecode/page_speed/PagespeedOutput.class',
            '<(pagespeed_proto_java_objroot)/com/googlecode/page_speed/PagespeedProtoFormatter.class',
          ],
	  # Assumes javac is in the classpath.
          'action': [
            'javac',
            '-d', '<(pagespeed_proto_java_objroot)',
            '-cp', '<(java_objroot)/protobuf.jar',
            '<(pagespeed_proto_java_srcroot)/com/googlecode/page_speed/PagespeedInput.java',
            '<(pagespeed_proto_java_srcroot)/com/googlecode/page_speed/PagespeedOutput.java',
            '<(pagespeed_proto_java_srcroot)/com/googlecode/page_speed/PagespeedProtoFormatter.java',
          ],
        },
      ],
    },
    {
      'target_name': 'pagespeed_proto_java_jar',
      'suppress_wildcard': 1,
      'type': 'none',
      'dependencies': [
        'pagespeed_proto_java_javac',
      ],
      'actions': [
        {
          'action_name': 'jar',
          'inputs': [
          ],
          'outputs': [
            '<(java_objroot)/pagespeed_proto.jar',
          ],
          'action': [
            'jar', 'cf', '<(java_objroot)/pagespeed_proto.jar',
            '-C', '<(pagespeed_proto_java_objroot)', '.'
	  ],
        },
      ],
    },
  ],
}
