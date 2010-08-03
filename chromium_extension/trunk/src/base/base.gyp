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
    'chromium_root': '<(DEPTH)/third_party/chromium/src',
  },
  'targets': [
    {
      'target_name': 'base',
      'type': '<(library)',
      'sources': [
        '<(chromium_root)/base/debug_util.cc',
        '<(chromium_root)/base/lock.cc',
        '<(chromium_root)/base/lock_impl_posix.cc',
        '<(chromium_root)/base/string_piece.cc',
        '<(chromium_root)/base/third_party/dmg_fp/dtoa.cc',
        '<(chromium_root)/base/third_party/dmg_fp/g_fmt.cc',

        # Use the modified files below instead of these originals:
        #    <(chromium_root)/base/debug_util_posix.cc
        #    <(chromium_root)/base/third_party/nspr/prtime.cc
        #    <(chromium_root)/base/safe_strerror_posix.cc
        #    <(chromium_root)/base/logging.cc
        #    <(chromium_root)/base/string16.cc
        #    <(chromium_root)/base/string_util.cc
        # When updating the version of Chromium that we use, be sure to check
        # for changes in these files.
        'debug_util_nacl.cc',
        'prtime_nacl.cc',
        'safe_strerror_nacl.cc',
        '<(DEPTH)/third_party/libpagespeed/src/base/logging.cc',
        '<(DEPTH)/third_party/libpagespeed/src/base/string16.cc',
        '<(DEPTH)/third_party/libpagespeed/src/base/string_util.cc',
      ],
      'cflags': [
        '-Wno-write-strings',
        '-Wno-error',
      ],
      'include_dirs': [
        '<(chromium_root)',
      ],
      'direct_dependent_settings': {
        'include_dirs': [
          '<(chromium_root)',
        ],
      },
    },
  ],
}
