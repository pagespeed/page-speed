# Copyright 2013 Google Inc.
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
#
# PageSpeed overrides for Chromium's common.gypi.
{
  'variables': {
    # Putting a variables dict inside another variables dict looks
    # kind of weird. This is done so that some variables are defined
    # as variables within the outer variables dict here. This is
    # necessary to get these variables defined for the conditions
    # within this variables dict that operate on these variables.
    'variables': {
      # Whether or not we are building for native client.
      'build_nacl%': 0,
    },

    # Copy conditionally-set variables out one scope.
    'build_nacl%': '<(build_nacl)',

    # TODO(bmcquade); these should be removed.
    'protobuf_gyp_path%': 'third_party/protobuf/protobuf.gyp',
    'protobuf_src_root%': 'third_party/protobuf/src',

    # Projects that depend on us may map instaweb to a different path,
    # so we need to make the instaweb src root path overridable.
    'instaweb_src_root%': 'net/instaweb',


    # Conditions that operate on our variables defined above.
    'conditions': [
      ['build_nacl==1', {
        # Disable position-independent code when building under Native
        # Client.
        'linux_fpic': 0,
      }],
    ],


    # Override a few Chromium variables:

    # Chromium uses system shared libraries on Linux by default
    # (Chromium already has transitive dependencies on these libraries
    # via gtk). We want to link these libraries into our binaries so
    # we change the default behavior.
    'use_system_libjpeg': 0,
    'use_system_libpng': 0,
    'use_system_zlib': 0,
  },
  'target_defaults': {
    # Make sure our shadow view of chromium source is available to
    # targets that don't explicitly declare their dependencies and
    # assume chromium source headers are available from the root
    # (third_party/modp_b64 is one such target).
    'include_dirs': [
      '<(DEPTH)/third_party/chromium/src',
    ],
  },
  'conditions': [
    ['os_posix==1 and OS!="mac"', {
      'target_defaults': {
        'ldflags': [
          # Fail to link if there are any undefined symbols.
          '-Wl,-z,defs',
        ],
      }
    }],
    ['OS=="win"', {
      'target_defaults': {
        # Remove the following defines, which are normally defined by
        # Chromium's common.gypi.
        'defines!': [
          # Chromium's common.gypi disables tr1. We need it for tr1
          # regex so remove their define to disable it.
          '_HAS_TR1=0',

          # Chromium disables exceptions in some environments, but our
          # use of tr1 regex requires exception support, so we have to
          # re-enable it here.
          '_HAS_EXCEPTIONS=0',
        ],
        'msvs_settings': {
          'VCCLCompilerTool': {
            'ExceptionHandling': '1',  # /EHsc
          },
        },
      },
    }]
  ],
}
