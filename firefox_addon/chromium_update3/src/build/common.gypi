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
    'conditions': [
      ['OS=="win"', {
        'xpcom_os': 'WINNT',
        'xpcom_compiler_abi': 'msvc',
      }],
      ['OS=="linux"', {
        'xpcom_os': 'Linux',
        'xpcom_compiler_abi': 'gcc3',
      }],
      ['OS=="mac"', {
        'xpcom_os': 'Darwin',
        'xpcom_compiler_abi': 'gcc3',
      }],
      ['target_arch=="ia32"', {
        'xpcom_cpu_arch': 'x86',
      }],
      ['target_arch=="x64"', {
        'xpcom_cpu_arch': 'x86_64',
      }],
    ],

    'variables': {
      # Version of xulrunner SDK we build against.
      'xulrunner_sdk_version%': 2,
    },

    'xulrunner_sdk_version%': '<(xulrunner_sdk_version)',

    # Make sure we link statically so everything gets linked into a
    # single shared object.
    'library': 'static_library',

    # We're building a shared library, so everything needs to be built
    # with Position-Independent Code.
    'linux_fpic': 1,
  },
  'includes': [
#    '../third_party/libpagespeed/src/build/common.gypi',
    'pagespeed-common.gypi',
  ],
  'conditions': [
    [ 'OS=="mac" and target_arch=="x64"', {
      'target_defaults': {
        'xcode_settings': {
          'ARCHS': 'x86_64',
          'OTHER_CFLAGS': [
            '-fPIC',
          ]
        }
      }
    }]
  ]
}
