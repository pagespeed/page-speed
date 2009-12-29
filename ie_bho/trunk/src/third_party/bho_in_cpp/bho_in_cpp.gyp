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
      'target_name': 'bho_in_cpp',
      'type': 'loadable_module',
      'variables': {
        'src_root': 'BHOinCPP',
      },
      'sources': [
        '<(src_root)/dll.def',
        '<(src_root)/ClassFactory.cpp',
        '<(src_root)/EventSink.cpp',
        '<(src_root)/ObjectWithSite.cpp',
        '<(src_root)/main.cpp',
      ],
      'include_dirs': [
        '<(src_root)',
      ],
    },
  ],
}
