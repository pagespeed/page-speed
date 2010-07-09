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
        '<(instaweb_root)/htmlparse/html_element.cc',
        '<(instaweb_root)/htmlparse/html_event.cc',
        '<(instaweb_root)/htmlparse/html_filter.cc',
        '<(instaweb_root)/htmlparse/html_lexer.cc',
        '<(instaweb_root)/htmlparse/html_node.cc',
        '<(instaweb_root)/htmlparse/html_parse.cc',
        '<(instaweb_root)/htmlparse/html_writer_filter.cc',
      ],
      'include_dirs': [
        '<(instaweb_src)',
      ],
      'direct_dependent_settings': {
        'include_dirs': [
          '<(instaweb_src)',
        ],
      },
      'export_dependent_settings': [
        'util',
        '<(DEPTH)/base/base.gyp:base',
      ],
    },
    {
      'target_name': 'rewriter',
      'type': '<(library)',
      'dependencies': [
        'util',
        '<(DEPTH)/base/base.gyp:base',
      ],
      'sources': [
        '<(instaweb_root)/rewriter/collapse_whitespace_filter.cc',
        '<(instaweb_root)/rewriter/elide_attributes_filter.cc',
        '<(instaweb_root)/rewriter/html_attribute_quote_removal.cc',
        '<(instaweb_root)/rewriter/remove_comments_filter.cc',
      ],
      'include_dirs': [
        '<(instaweb_src)',
      ],
      'direct_dependent_settings': {
        'include_dirs': [
          '<(instaweb_src)',
        ],
      },
      'export_dependent_settings': [
        'util',
        '<(DEPTH)/base/base.gyp:base',
      ],
    },
    {
      'target_name': 'util',
      'type': '<(library)',
      'dependencies': [
        '<(DEPTH)/base/base.gyp:base',
        '<(DEPTH)/third_party/base64/base64.gyp:base64',
        '<(DEPTH)/third_party/protobuf2/protobuf.gyp:protobuf',
      ],
      'sources': [
        '<(instaweb_root)/util/file_message_handler.cc',
        '<(instaweb_root)/util/message_handler.cc',
        '<(instaweb_root)/util/string_writer.cc',
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
      'export_dependent_settings': [
        '<(DEPTH)/base/base.gyp:base',
        '<(DEPTH)/third_party/base64/base64.gyp:base64',
        '<(DEPTH)/third_party/protobuf2/protobuf.gyp:protobuf',
      ],
    },
  ],
}
