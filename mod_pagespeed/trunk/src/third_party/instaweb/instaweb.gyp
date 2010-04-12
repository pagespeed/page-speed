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
        'util',
        '<(DEPTH)/base/base.gyp:base',
      ],
      'sources': [
        '<(instaweb_root)/rewriter/add_head_filter.cc',
        '<(instaweb_root)/rewriter/base_tag_filter.cc',
        '<(instaweb_root)/rewriter/css_sprite_filter.cc',
        '<(instaweb_root)/rewriter/file_input_resource.cc',
        '<(instaweb_root)/rewriter/filename_output_resource.cc',
        '<(instaweb_root)/rewriter/filename_resource_manager.cc',
        '<(instaweb_root)/rewriter/hash_output_resource.cc',
        '<(instaweb_root)/rewriter/hash_resource_manager.cc',
        '<(instaweb_root)/rewriter/img_rewrite_filter.cc',
        '<(instaweb_root)/rewriter/input_resource.cc',
        '<(instaweb_root)/rewriter/outline_filter.cc',
        '<(instaweb_root)/rewriter/output_resource.cc',
        '<(instaweb_root)/rewriter/resource_manager.cc',
        '<(instaweb_root)/rewriter/rewrite_driver.cc',
#        '<(instaweb_root)/rewriter/rewriter_main.cc',
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
        '<(DEPTH)/base/base.gyp:base',
        '<(DEPTH)/third_party/base64/base64.gyp:base64',
      ],
      'sources': [
        '<(instaweb_root)/util/abstract_mutex.cc',
        '<(instaweb_root)/util/cache_interface.cc',
        '<(instaweb_root)/util/dummy_url_fetcher.cc',
        '<(instaweb_root)/util/file_message_handler.cc',
        '<(instaweb_root)/util/file_system.cc',
        '<(instaweb_root)/util/file_writer.cc',
        '<(instaweb_root)/util/hasher.cc',
        '<(instaweb_root)/util/lru_cache.cc',
        '<(instaweb_root)/util/message_handler.cc',
        '<(instaweb_root)/util/meta_data.cc',
        '<(instaweb_root)/util/pthread_mutex.cc',
        '<(instaweb_root)/util/simple_meta_data.cc',
        '<(instaweb_root)/util/stdio_file_system.cc',
        '<(instaweb_root)/util/string_writer.cc',
        '<(instaweb_root)/util/threadsafe_cache.cc',
        '<(instaweb_root)/util/url_fetcher.cc',
        '<(instaweb_root)/util/wget_url_fetcher.cc',
        '<(instaweb_root)/util/writer.cc',
      ],
      'include_dirs': [
        '<(DEPTH)',
        '<(instaweb_src)',
      ],
      'direct_dependent_settings': {
        'include_dirs': [
          '<(DEPTH)',
          '<(instaweb_src)',
        ],
      },
    },

  ],
}
