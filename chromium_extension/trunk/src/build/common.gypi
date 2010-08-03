# Copyright 2010 Google Inc.
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
    '../third_party/libpagespeed/src/build/common.gypi',
  ],
  'target_defaults': {
    'include_dirs': [
      '<(DEPTH)/build/nacl_header_stubs',
    ],
    'conditions': [
      ['target_arch=="ia32"', {
        'cflags': [
          '-m32',
        ],
        'ldflags': [
          '-m32',
        ],
      }],
      ['target_arch=="x64"', {
        'cflags': [
          '-m64',
        ],
        'ldflags': [
          '-m64',
        ],
      }],
    ],
  },
}
