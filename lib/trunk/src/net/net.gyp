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
  'variables': {
    'chromium_code': 1,
  },
  'targets': [
    {
      'target_name': 'net_base',
      'type': '<(library)',
      'dependencies': [
        '../base/base.gyp:base',
        '../build/temp_gyp/googleurl.gyp:googleurl',
      ],
      'sources': [
        'base/registry_controlled_domain.cc',
      ],
      'export_dependent_settings': [
        '../base/base.gyp:base',
      ],
      'include_dirs': [
        '<(DEPTH)/third_party/chromium/src/net/base',
      ],
    },
  ],
}

# Local Variables:
# tab-width:2
# indent-tabs-mode:nil
# End:
# vim: set expandtab tabstop=2 shiftwidth=2:
