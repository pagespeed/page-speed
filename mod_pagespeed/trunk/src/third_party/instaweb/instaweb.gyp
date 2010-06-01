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
    'instaweb_src': '<(DEPTH)/third_party/instaweb/src',
    'instaweb_root': '<(instaweb_src)/net/instaweb',
    'mod_spdy_src': '<(DEPTH)/third_party/mod_spdy/src',
    'protobuf_src': '<(DEPTH)/third_party/protobuf2/src/src',
  },
  'targets': [
    {
      'target_name': 'htmlparse',
      'type': '<(library)',
      'dependencies': [
        'util',
        '<(DEPTH)/base/base.gyp:base',
      ],
      'sources': [
        '<(instaweb_root)/htmlparse/empty_html_filter.cc',
        '<(instaweb_root)/htmlparse/file_driver.cc',
        '<(instaweb_root)/htmlparse/file_statistics_log.cc',
        '<(instaweb_root)/htmlparse/html_element.cc',
        '<(instaweb_root)/htmlparse/html_event.cc',
        '<(instaweb_root)/htmlparse/html_filter.cc',
        '<(instaweb_root)/htmlparse/html_lexer.cc',
        '<(instaweb_root)/htmlparse/html_parse.cc',
        '<(instaweb_root)/htmlparse/html_writer_filter.cc',
        '<(instaweb_root)/htmlparse/logging_html_filter.cc',
        '<(instaweb_root)/htmlparse/null_filter.cc',
        '<(instaweb_root)/htmlparse/statistics_log.cc',
      ],
      'include_dirs': [
        '<(instaweb_src)',
      ],
      'direct_dependent_settings': {
        'include_dirs': [
          '<(instaweb_src)',
        ],
      },
    },
    {
      'target_name': 'rewriter',
      'type': '<(library)',
      'dependencies': [
        'rewrite_pb',
        'util',
        '<(DEPTH)/base/base.gyp:base',
        '<(DEPTH)/third_party/libpagespeed/src/pagespeed/pagespeed.gyp:*',
      ],
      'sources': [
        '<(instaweb_root)/rewriter/add_head_filter.cc',
        '<(instaweb_root)/rewriter/base_tag_filter.cc',
        '<(instaweb_root)/rewriter/cache_extender.cc',
        '<(instaweb_root)/rewriter/css_filter.cc',
        '<(instaweb_root)/rewriter/css_combine_filter.cc',
        '<(instaweb_root)/rewriter/file_input_resource.cc',
        '<(instaweb_root)/rewriter/filename_output_resource.cc',
        '<(instaweb_root)/rewriter/filename_resource_manager.cc',
        '<(instaweb_root)/rewriter/hash_output_resource.cc',
        '<(instaweb_root)/rewriter/hash_resource_manager.cc',
        '<(instaweb_root)/rewriter/html_attribute_quote_removal.cc',
        '<(instaweb_root)/rewriter/image.cc',
        '<(instaweb_root)/rewriter/img_filter.cc',
        '<(instaweb_root)/rewriter/img_rewrite_filter.cc',
        '<(instaweb_root)/rewriter/input_resource.cc',
        '<(instaweb_root)/rewriter/outline_filter.cc',
        '<(instaweb_root)/rewriter/output_resource.cc',
        '<(instaweb_root)/rewriter/resource_manager.cc',
        '<(instaweb_root)/rewriter/rewrite_driver.cc',
        '<(instaweb_root)/rewriter/rewrite_filter.cc',
        '<(instaweb_root)/rewriter/url_input_resource.cc',
      ],
      'include_dirs': [
        '<(instaweb_src)',
      ],
      'direct_dependent_settings': {
        'include_dirs': [
          '<(instaweb_src)',
        ],
      },
    },
    {
      'target_name': 'util',
      'type': '<(library)',
      'dependencies': [
        'util_pb',
        '<(DEPTH)/base/base.gyp:base',
        '<(DEPTH)/third_party/base64/base64.gyp:base64',
        '<(DEPTH)/third_party/libpagespeed/src/pagespeed/pagespeed.gyp:*',
      ],
      'sources': [
        '<(instaweb_root)/util/abstract_mutex.cc',
        '<(instaweb_root)/util/cache_interface.cc',
        '<(instaweb_root)/util/cache_url_async_fetcher.cc',
        '<(instaweb_root)/util/cache_url_fetcher.cc',
        '<(instaweb_root)/util/content_type.cc',
        '<(instaweb_root)/util/fake_url_async_fetcher.cc',
        '<(instaweb_root)/util/file_cache.cc',
        '<(instaweb_root)/util/file_message_handler.cc',
        '<(instaweb_root)/util/file_system.cc',
        '<(instaweb_root)/util/file_writer.cc',
        '<(instaweb_root)/util/filename_encoder.cc',
        '<(instaweb_root)/util/hasher.cc',
        '<(instaweb_root)/util/http_cache.cc',
        '<(instaweb_root)/util/lru_cache.cc',
        '<(instaweb_root)/util/message_handler.cc',
        '<(instaweb_root)/util/meta_data.cc',
        '<(instaweb_root)/util/mock_hasher.cc',
        '<(instaweb_root)/util/pthread_mutex.cc',
        '<(instaweb_root)/util/simple_meta_data.cc',
        '<(instaweb_root)/util/stdio_file_system.cc',
        '<(instaweb_root)/util/string_buffer.cc',
        '<(instaweb_root)/util/string_buffer_writer.cc',
        '<(instaweb_root)/util/string_writer.cc',
        '<(instaweb_root)/util/string_util.cc',
        '<(instaweb_root)/util/threadsafe_cache.cc',
        '<(instaweb_root)/util/timer.cc',
        '<(instaweb_root)/util/url_async_fetcher.cc',
        '<(instaweb_root)/util/url_fetcher.cc',
        '<(instaweb_root)/util/wget_url_fetcher.cc',
        '<(instaweb_root)/util/writer.cc',
      ],
      'include_dirs': [
        '<(DEPTH)',
        '<(instaweb_src)',
        '<(mod_spdy_src)',
        '<(protobuf_src)',
      ],
      'direct_dependent_settings': {
        'include_dirs': [
          '<(DEPTH)',
          '<(instaweb_src)',
          '<(mod_spdy_src)',
          '<(protobuf_src)',
        ],
      },
    },
    {
      'target_name': 'rewrite_pb',
      'type': '<(library)',
      'hard_dependency': 1,
      'dependencies': [
          '<(DEPTH)/third_party/protobuf2/protobuf.gyp:protobuf',
          '<(DEPTH)/third_party/protobuf2/protobuf.gyp:protoc',
       ],
      'actions': [
        {
          'action_name': 'my_rewrite_proto',
          'inputs': [
            '<(PRODUCT_DIR)/<(EXECUTABLE_PREFIX)protoc<(EXECUTABLE_SUFFIX)',
            '<(instaweb_root)/rewriter/rewrite.proto',
          ],
          'outputs': [
            '<(SHARED_INTERMEDIATE_DIR)/net/instaweb/rewriter/rewrite.pb.cc',
            '<(SHARED_INTERMEDIATE_DIR)/net/instaweb/rewriter/rewrite.pb.h',
          ],
          'dependencies': [
            '<(DEPTH)/third_party/protobuf2/protobuf.gyp:protoc',
          ],
          'action': [
            '<(PRODUCT_DIR)/<(EXECUTABLE_PREFIX)protoc<(EXECUTABLE_SUFFIX)',
            '<(instaweb_root)/rewriter/rewrite.proto',
            '--proto_path=<(instaweb_src)',
            '--cpp_out=<(SHARED_INTERMEDIATE_DIR)',
          ],
        },
      ],
      'sources': [
        '<(SHARED_INTERMEDIATE_DIR)/net/instaweb/rewriter/rewrite.pb.cc',
      ],
      'include_dirs': [
        '<(SHARED_INTERMEDIATE_DIR)',
      ],
      'direct_dependent_settings': {
        'include_dirs': [
          '<(SHARED_INTERMEDIATE_DIR)',
        ],
      },
      'export_dependent_settings': [
        '<(DEPTH)/third_party/protobuf2/protobuf.gyp:protobuf',
      ]
    },
    {
      'target_name': 'util_pb',
      'type': '<(library)',
      'hard_dependency': 1,
      'dependencies': [
          '<(DEPTH)/third_party/protobuf2/protobuf.gyp:protobuf',
          '<(DEPTH)/third_party/protobuf2/protobuf.gyp:protoc',
       ],
      'actions': [
        {
          'action_name': 'my_util_proto',
          'inputs': [
            '<(PRODUCT_DIR)/<(EXECUTABLE_PREFIX)protoc<(EXECUTABLE_SUFFIX)',
            '<(instaweb_root)/util/util.proto',
          ],
          'outputs': [
            '<(SHARED_INTERMEDIATE_DIR)/net/instaweb/util/util.pb.cc',
            '<(SHARED_INTERMEDIATE_DIR)/net/instaweb/util/util.pb.h',
          ],
          'dependencies': [
            '<(DEPTH)/third_party/protobuf2/protobuf.gyp:protoc',
          ],
          'action': [
            '<(PRODUCT_DIR)/<(EXECUTABLE_PREFIX)protoc<(EXECUTABLE_SUFFIX)',
            '<(instaweb_root)/util/util.proto',
            '--proto_path=<(instaweb_src)',
            '--cpp_out=<(SHARED_INTERMEDIATE_DIR)',
          ],
        },
      ],
      'sources': [
        '<(SHARED_INTERMEDIATE_DIR)/net/instaweb/util/util.pb.cc',
      ],
      'include_dirs': [
        '<(SHARED_INTERMEDIATE_DIR)',
      ],
      'direct_dependent_settings': {
        'include_dirs': [
          '<(SHARED_INTERMEDIATE_DIR)',
        ],
      },
      'export_dependent_settings': [
        '<(DEPTH)/third_party/protobuf2/protobuf.gyp:protobuf',
      ]
    },
  ],
}
