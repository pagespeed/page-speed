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
    # We must build for 10.4 for compatibility with Firefox.
    'mac_deployment_target': '10.4',
  },
  'includes': [
    '../third_party/libpagespeed/src/build/common.gypi',
  ],
  'target_defaults': {
    'defines': [
      # break dependency on obsolete nspr headers
      'NO_NSPR_10_SUPPORT',
    ],
    'conditions': [
      ['OS == "linux"', {
        'cflags': [
          # We're building a shared library, so everything needs to be built
          # with Position-Independent Code.
          '-fPIC',
        ],
      }],
    ],
  },
}
