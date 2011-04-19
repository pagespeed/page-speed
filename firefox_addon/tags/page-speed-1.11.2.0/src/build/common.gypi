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
      [ 'target_arch=="ia32"', {
        # We build for 10.4 for compatibility with Firefox 3.x.
        'mac_deployment_target': '10.4',
      }, {
        # However mac x64 requires 10.5 as a minimum.
        'mac_deployment_target': '10.5',
      }]
    ],

    # Make sure we link statically so everything gets linked into a
    # single shared object.
    'library': 'static_library',

    # We're building a shared library, so everything needs to be built
    # with Position-Independent Code.
    'linux_fpic': 1,
  },
  'includes': [
    '../third_party/libpagespeed/src/build/common.gypi',
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
