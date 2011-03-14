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
    # Make sure we link statically so everything gets linked into a
    # single shared object.
    'library': 'static_library',

    # The nacl toolchain fails to build valid nexes when we enable gc
    # sections, at least on 64 bit builds. TODO: revisit this to see
    # if a newer nacl toolchain supports it.
    'no_gc_sections': 1,

    # We're building a shared library, so everything needs to be built
    # with Position-Independent Code.
    'linux_fpic': 1,
  },
  'includes': [
    '../third_party/libpagespeed/src/build/common.gypi',
  ],
  # 'target_defaults': {
  #   'include_dirs': [
  #     '<(DEPTH)/build/nacl_header_stubs',
  #   ],
  # },
}
