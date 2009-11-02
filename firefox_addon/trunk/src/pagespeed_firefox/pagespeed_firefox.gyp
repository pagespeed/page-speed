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
    'xulrunner_sdk_root': '<(DEPTH)/third_party/xulrunner-sdk',
  },
  'targets': [
    {
      'target_name': 'xulrunner_sdk',
      'type': 'none',
      'direct_dependent_settings': {
        'include_dirs': [
          '<(xulrunner_sdk_root)/include/dom',
          '<(xulrunner_sdk_root)/include/layout',
          '<(xulrunner_sdk_root)/include/necko',
          '<(xulrunner_sdk_root)/include/nspr',
          '<(xulrunner_sdk_root)/include/string',
          '<(xulrunner_sdk_root)/include/xpcom',
        ],
        'conditions': [
          ['OS == "linux"', {
            'cflags': [
              '-fshort-wchar',
              '-include', '<(xulrunner_sdk_root)/include/xpcom/xpcom-config.h',
            ],
            'ldflags': [  # See https://developer.mozilla.org/en/XPCOM_Glue
              '-L<(xulrunner_sdk_root)/lib',
              '-L<(xulrunner_sdk_root)/bin',
              '-Wl,-rpath-link,<(xulrunner_sdk_root)/bin',
              '-Wl,--whole-archive',
              '-lxpcomglue_s',
              '-lxpcom',
              '-lnspr4',
              '-Wl,--no-whole-archive',
            ],
          }],
        ],
      },
    },
    {
      'target_name': 'pagespeed_firefox_profile_pb',
      'type': '<(library)',
      'hard_dependency': 1,
      'dependencies': [
        '<(DEPTH)/third_party/protobuf2/protobuf.gyp:protobuf',
        '<(DEPTH)/third_party/protobuf2/protobuf.gyp:protoc',
      ],
      'actions': [
        {
          'action_name': 'generate_pagespeed_firefox_profile_pb',
          'variables': {
            'proto_path': 'protobuf',
          },
          'inputs': [
            '<(PRODUCT_DIR)/<(EXECUTABLE_PREFIX)protoc<(EXECUTABLE_SUFFIX)',
            '<(proto_path)/activity/profile.proto',
          ],
          'outputs': [
            '<(SHARED_INTERMEDIATE_DIR)/activity/profile.pb.cc',
            '<(SHARED_INTERMEDIATE_DIR)/activity/profile.pb.h',
          ],
          'dependencies': [
            '<(DEPTH)/third_party/protobuf2/protobuf.gyp:protoc',
          ],
          'action': [
            '<(PRODUCT_DIR)/<(EXECUTABLE_PREFIX)protoc<(EXECUTABLE_SUFFIX)',
            '<(proto_path)/activity/profile.proto',
            '--proto_path=<(proto_path)',
            '--cpp_out=<(SHARED_INTERMEDIATE_DIR)',
          ],
        },
      ],
      'sources': [
        '<(SHARED_INTERMEDIATE_DIR)/activity/profile.pb.cc',
      ],
      'include_dirs': [
        '<(SHARED_INTERMEDIATE_DIR)',
      ],
      'direct_dependent_settings': {
        'include_dirs': [
          '<(SHARED_INTERMEDIATE_DIR)/activity',
        ],
      },
      'export_dependent_settings': [
        '<(DEPTH)/third_party/protobuf2/protobuf.gyp:protobuf',
      ]
    },
    {
      'target_name': 'pagespeed_firefox_activity',
      'type': '<(library)',
      'variables': {
        'activity_root': 'cpp/activity',
        'mozilla_idl_root': '<(DEPTH)/third_party/mozilla/idl',
      },
      'dependencies': [
        'xulrunner_sdk',
        'pagespeed_firefox_profile_pb',
        '<(DEPTH)/base/base.gyp:base',
      ],
      'actions': [
        {
          'action_name': 'generate_IActivityProfiler',
          'inputs': [
            'idl/IActivityProfiler.idl',
          ],
          'outputs': [
            '<(SHARED_INTERMEDIATE_DIR)/IActivityProfiler.h',
          ],
          'action': [
            '<(xulrunner_sdk_root)/bin/xpidl',
            '-m', 'header',
            '-I', '<(xulrunner_sdk_root)/idl',
            '-e', '<(_outputs)',
            '<(_inputs)',
          ],
        },
        {
          'action_name': 'generate_jsdIDebuggerService_3_0',
          'inputs': [
            '<(mozilla_idl_root)/jsdIDebuggerService_3_0.idl',
          ],
          'outputs': [
            '<(SHARED_INTERMEDIATE_DIR)/jsdIDebuggerService_3_0.h',
          ],
          'action': [
            '<(xulrunner_sdk_root)/bin/xpidl',
            '-m', 'header',
            '-I', '<(xulrunner_sdk_root)/idl',
            '-e', '<(_outputs)',
            '<(_inputs)',
          ],
        },
        {
          'action_name': 'generate_jsdIDebuggerService_3_5',
          'inputs': [
            '<(mozilla_idl_root)/jsdIDebuggerService_3_5.idl',
          ],
          'outputs': [
            '<(SHARED_INTERMEDIATE_DIR)/jsdIDebuggerService_3_5.h',
          ],
          'action': [
            '<(xulrunner_sdk_root)/bin/xpidl',
            '-m', 'header',
            '-I', '<(xulrunner_sdk_root)/idl',
            '-e', '<(_outputs)',
            '<(_inputs)',
          ],
        },
        {
          'action_name': 'generate_jsdIDebuggerService_3_6',
          'inputs': [
            '<(mozilla_idl_root)/jsdIDebuggerService_3_6.idl',
          ],
          'outputs': [
            '<(SHARED_INTERMEDIATE_DIR)/jsdIDebuggerService_3_6.h',
          ],
          'action': [
            '<(xulrunner_sdk_root)/bin/xpidl',
            '-m', 'header',
            '-I', '<(xulrunner_sdk_root)/idl',
            '-e', '<(_outputs)',
            '<(_inputs)',
          ],
        },
      ],
      'sources': [
        '<(activity_root)/basic_tree_view.cc',
        '<(activity_root)/call_graph.cc',
        '<(activity_root)/call_graph_metadata.cc',
        '<(activity_root)/call_graph_profile.cc',
        '<(activity_root)/call_graph_profile_snapshot.cc',
        '<(activity_root)/call_graph_timeline_event_set.cc',
        '<(activity_root)/call_graph_timeline_visitor.cc',
        '<(activity_root)/call_graph_util.cc',
        '<(activity_root)/call_graph_visit_filter_interface.cc',
        '<(activity_root)/call_graph_visitor_interface.cc',
        '<(activity_root)/check_abort.cc',
        '<(activity_root)/check_gecko.cc',
        '<(activity_root)/clock.cc',
        '<(activity_root)/delayable_function_tree_view_delegate.cc',
        '<(activity_root)/find_first_invocations_visitor.cc',
        '<(activity_root)/http_activity_distributor.cc',
        '<(activity_root)/jsd_call_hook.cc',
        '<(activity_root)/jsd_function_info.cc',
        '<(activity_root)/jsd_script_hook.cc',
        '<(activity_root)/jsd_wrapper.cc',
        '<(activity_root)/jsd_wrapper_3_0.cc',
        '<(activity_root)/jsd_wrapper_3_5.cc',
        '<(activity_root)/jsd_wrapper_3_6.cc',
        '<(activity_root)/profiler.cc',
        '<(activity_root)/profiler_event.cc',
        '<(activity_root)/profiler_runnables.cc',
        '<(activity_root)/timer.cc',
        '<(activity_root)/uncalled_function_tree_view_delegate.cc',
      ],
      'include_dirs': [
        '<(SHARED_INTERMEDIATE_DIR)',
        '<(activity_root)',
      ],
    },
    {
      'target_name': 'pagespeed_firefox_image_compressor',
      'type': '<(library)',
      'variables': {
        'image_compressor_root': 'cpp/image_compressor',
      },
      'dependencies': [
        'xulrunner_sdk',
        '<(DEPTH)/base/base.gyp:base',
        '<(DEPTH)/third_party/optipng/optipng.gyp:optipng',
        '<(DEPTH)/third_party/libjpeg/libjpeg.gyp:libjpeg',
      ],
      'actions': [
        {
          'action_name': 'generate_IImageCompressor',
          'inputs': [
            'idl/IImageCompressor.idl',
          ],
          'outputs': [
            '<(SHARED_INTERMEDIATE_DIR)/IImageCompressor.h',
          ],
          'action': [
            '<(xulrunner_sdk_root)/bin/xpidl',
            '-m', 'header',
            '-I', '<(xulrunner_sdk_root)/idl',
            '-e', '<(_outputs)',
            '<(_inputs)',
          ],
        },
      ],
      'defines': [
        # Disable PNG optimization for now; we're having issues getting optipng
        # to not crash when we build using gyp instead of mozilla.
        'PAGESPEED_DISABLE_PNG_OPTIMIZATION',
      ],
      'sources': [
        '<(image_compressor_root)/image_compressor.cc',
        '<(image_compressor_root)/jpeg_optimizer.cc',
        '<(image_compressor_root)/png_optimizer.cc',
      ],
      'include_dirs': [
        '<(image_compressor_root)',
        '<(SHARED_INTERMEDIATE_DIR)',
      ],
      'export_dependent_settings': [
        '<(DEPTH)/base/base.gyp:base',
        '<(DEPTH)/third_party/optipng/optipng.gyp:optipng',
      ],
    },
    {
      'target_name': 'pagespeed_firefox_library_rules',
      'type': '<(library)',
      'variables': {
        'library_rules_root': 'cpp/pagespeed',
      },
      'dependencies': [
        'xulrunner_sdk',
        '<(DEPTH)/base/base.gyp:base',
      ],
      'actions': [
        {
          'action_name': 'generate_IPageSpeedRules_h',
          'inputs': [
            'idl/IPageSpeedRules.idl',
          ],
          'outputs': [
            '<(SHARED_INTERMEDIATE_DIR)/IPageSpeedRules.h',
          ],
          'action': [
            '<(xulrunner_sdk_root)/bin/xpidl',
            '-m', 'header',
            '-I', '<(xulrunner_sdk_root)/idl',
            '-e', '<(_outputs)',
            '<(_inputs)',
          ],
        },
      ],
      'sources': [
        '<(library_rules_root)/pagespeed_rules.cc',
      ],
      'include_dirs': [
        '<(library_rules_root)',
        '<(SHARED_INTERMEDIATE_DIR)',
      ],
      'export_dependent_settings': [
        '<(DEPTH)/base/base.gyp:base',
      ],
    },
    {
      'target_name': 'pagespeed_firefox_module',
      'type': 'loadable_module',
      'variables': {
        'src_root': 'cpp',
      },
      'dependencies': [
        'xulrunner_sdk',
        '<(DEPTH)/base/base.gyp:base',
        'pagespeed_firefox_activity',
        'pagespeed_firefox_image_compressor',
        'pagespeed_firefox_library_rules',
      ],
      'sources': [
        '<(src_root)/pagespeed/pagespeed_module.cc',
      ],
      'include_dirs': [
        '<(src_root)',
        '<(src_root)/pagespeed',
        '<(SHARED_INTERMEDIATE_DIR)',
      ],
    },
  ],
}
