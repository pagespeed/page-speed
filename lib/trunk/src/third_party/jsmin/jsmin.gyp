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
  'targets': [
    {
      'target_name': 'jsmin',
      'type': '<(library)',
      'dependencies': [
        '<(DEPTH)/base/base.gyp:base',
      ],
      'sources': [
        'cpp/jsmin.cc',
      ],
      'include_dirs': [
        '../..',
      ],
      'direct_dependent_settings': {
        'include_dirs': [
          '../..',
        ],
      },
    },
    {
      'target_name': 'jsmin_bin',
      'type': 'executable',
      'sources': [
        'jsmin.c',
      ],
    },
    {
      'target_name': 'jsmin_test',
      'type': 'executable',
      'dependencies': [
        'jsmin',
        '<(DEPTH)/testing/gtest.gyp:gtest',
        '<(DEPTH)/testing/gtest.gyp:gtestmain',
      ],
      'sources': [
        'cpp/jsmin_test.cc',
      ],
    },
  ],
}
