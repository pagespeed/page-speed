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
      'sources': [
        '<(instaweb_root)/htmlparse/empty_html_filter.cc',
        '<(instaweb_root)/htmlparse/file_driver.cc',
        '<(instaweb_root)/htmlparse/file_message_handler.cc',
        '<(instaweb_root)/htmlparse/file_system.cc',
        '<(instaweb_root)/htmlparse/file_writer.cc',
        '<(instaweb_root)/htmlparse/html_element.cc',
        '<(instaweb_root)/htmlparse/html_event.cc',
        '<(instaweb_root)/htmlparse/html_event.h',
        '<(instaweb_root)/htmlparse/html_filter.cc',
        '<(instaweb_root)/htmlparse/html_lexer.cc',
        '<(instaweb_root)/htmlparse/html_lexer.h',
        '<(instaweb_root)/htmlparse/html_parse.cc',
        '<(instaweb_root)/htmlparse/html_writer_filter.cc',
        '<(instaweb_root)/htmlparse/message_handler.cc',
        '<(instaweb_root)/htmlparse/null_filter.cc',
        '<(instaweb_root)/htmlparse/null_filter.h',
        '<(instaweb_root)/htmlparse/stdio_file_system.cc',
        '<(instaweb_root)/htmlparse/writer.cc',
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
      'sources': [
        '<(instaweb_root)/rewriter/add_head_filter.cc',
        '<(instaweb_root)/rewriter/base_tag_filter.cc',
        '<(instaweb_root)/rewriter/css_sprite_filter.cc',
        '<(instaweb_root)/rewriter/file_resource.cc',
        '<(instaweb_root)/rewriter/file_resource_manager.cc',
        '<(instaweb_root)/rewriter/outline_filter.cc',
        '<(instaweb_root)/rewriter/outline_resource.cc',
        '<(instaweb_root)/rewriter/resource.cc',
        '<(instaweb_root)/rewriter/resource_manager.cc',
        '<(instaweb_root)/rewriter/rewrite_driver.cc',
        '<(instaweb_root)/rewriter/sprite_resource.cc',
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
  ],
}
