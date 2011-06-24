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
    'xulrunner_sdk_root': '<(DEPTH)/third_party/xulrunner-sdk/<(xulrunner_sdk_version)',
    'xulrunner_sdk_os_root': '<(xulrunner_sdk_root)/arch/<(OS)',
    'xulrunner_sdk_arch_root': '<(xulrunner_sdk_os_root)/<(target_arch)',
  },
  'targets': [
    {
      'target_name': 'xulrunner_sdk',
      'type': 'none',
      'direct_dependent_settings': {
        'include_dirs': [
          '<(SHARED_INTERMEDIATE_DIR)',  # For headers generated from idl files
          '<(xulrunner_sdk_root)/include',
          '<(xulrunner_sdk_arch_root)/include',
        ],
        'defines': [
          'MOZ_NO_MOZALLOC',
          'NO_NSPR_10_SUPPORT',
        ],
        'conditions': [  # See https://developer.mozilla.org/en/XPCOM_Glue
          ['OS == "linux"', {
            'cflags': [
              '-fshort-wchar',
              '-include', 'third_party/xulrunner-sdk/<(xulrunner_sdk_arch_root)/include/mozilla-config.h',
              '-include', 'third_party/xulrunner-sdk/<(xulrunner_sdk_arch_root)/include/xpcom-config.h',
            ],
            'ldflags': [
              '-Lthird_party/xulrunner-sdk/<(xulrunner_sdk_arch_root)/lib',
            ],
            'link_settings': {
              'libraries': [
                '-lxpcomglue_s_nomozalloc',
                '-lxpcom',
                '-lnspr4',
            ]},
          }],
          ['OS == "mac"', {
            'link_settings': {
              'libraries': [
                'libxpcomglue_s_nomozalloc.a',
                'libxpcom.dylib',
                'libnspr4.dylib',
              ],
            },
            'xcode_settings': {
              'LIBRARY_SEARCH_PATHS': [
                # This needs to be relative to the code that depends
                # upon it, so we're forced to prepend a bogus
                # directory (cpp) in order to make this work relative
                # to the pagespeed_firefox directory.
                'cpp/<(xulrunner_sdk_arch_root)/lib',
              ],
              'OTHER_CFLAGS': [
                '-fshort-wchar',
                '-include cpp/<(xulrunner_sdk_arch_root)/include/mozilla-config.h',
                '-include cpp/<(xulrunner_sdk_arch_root)/include/xpcom-config.h',
              ],
            },
          }],
          ['OS == "win"', {
            'cflags': [
              '/FI "mozilla-config.h"',
              '/FI "xpcom-config.h"',
              '/Zc:wchar_t-',
            ],
            'defines': [
              'XP_WIN',
            ],
            'link_settings': {
              'libraries': [
                '<(xulrunner_sdk_arch_root)/lib/xpcomglue_s_nomozalloc.lib',
                '<(xulrunner_sdk_arch_root)/lib/xpcom.lib',
                '<(xulrunner_sdk_arch_root)/lib/nspr4.lib',
              ],
            },
          }]
        ],
      },
    },
  ],
}
