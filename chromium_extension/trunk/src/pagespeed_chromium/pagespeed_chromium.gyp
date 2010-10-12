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
    'libpagespeed_root': '<(DEPTH)/third_party/libpagespeed/src',
  },
  'targets': [
    {
      'target_name': 'pagespeed.nexe',
      'type': 'executable',
      'dependencies': [
        '<(DEPTH)/base/base.gyp:base',
        '<(libpagespeed_root)/pagespeed/core/core.gyp:pagespeed_core',
        '<(libpagespeed_root)/pagespeed/pagespeed.gyp:pagespeed_library',
        '<(libpagespeed_root)/pagespeed/filters/filters.gyp:pagespeed_filters',
        '<(libpagespeed_root)/pagespeed/formatters/formatters.gyp:pagespeed_formatters',
        '<(libpagespeed_root)/pagespeed/har/har.gyp:pagespeed_har',
      ],
      'sources': [
        'npp_gate.cc',
        'pagespeed_chromium.cc',
        'pagespeed_module.cc',
      ],
      'ldflags': [
        '-lgoogle_nacl_imc',
        '-lgoogle_nacl_npruntime',
        '-lpthread',
        '-lsrpc',
      ],
    },
    {
      'target_name': 'pagespeed_extension',
      'type': 'none',
      'dependencies': [
        'pagespeed.nexe',
      ],
      'actions': [
        {
          'action_name': 'make_dir',
          'inputs': [],
          'outputs': [
            '<(PRODUCT_DIR)/pagespeed',
          ],
          'action': [
            'mkdir', '<@(_outputs)',
          ],
        },
        {
          'action_name': 'copy_files',
          'inputs': [
            '<(PRODUCT_DIR)/pagespeed',
            'extension_files/background.html',
            'extension_files/background.js',
            'extension_files/errorRedDot.png',
            'extension_files/icon.png',
            'extension_files/manifest.json',
            'extension_files/pagespeed.css',
            'extension_files/pagespeed.html',
            'extension_files/pagespeed.js',
            'extension_files/pagespeed-panel.html',
            'extension_files/spinner.gif',
            'extension_files/successGreenDot.png',
            'extension_files/warningOrangeDot.png',
          ],
          'outputs': [
            '<(PRODUCT_DIR)/pagespeed/background.html',
            '<(PRODUCT_DIR)/pagespeed/background.js',
            '<(PRODUCT_DIR)/pagespeed/errorRedDot.png',
            '<(PRODUCT_DIR)/pagespeed/icon.png',
            '<(PRODUCT_DIR)/pagespeed/manifest.json',
            '<(PRODUCT_DIR)/pagespeed/pagespeed.css',
            '<(PRODUCT_DIR)/pagespeed/pagespeed.html',
            '<(PRODUCT_DIR)/pagespeed/pagespeed.js',
            '<(PRODUCT_DIR)/pagespeed/pagespeed-panel.html',
            '<(PRODUCT_DIR)/pagespeed/spinner.gif',
            '<(PRODUCT_DIR)/pagespeed/successGreenDot.png',
            '<(PRODUCT_DIR)/pagespeed/warningOrangeDot.png',
          ],
          'action': [
            'cp', '-t', '<@(_inputs)',
          ],
        },
        {
          'action_name': 'copy_nexe',
          'inputs': [
            '<(PRODUCT_DIR)/pagespeed',
            '<(PRODUCT_DIR)/pagespeed.nexe',
          ],
          'outputs': [
            '<(PRODUCT_DIR)/pagespeed/pagespeed_<(target_arch).nexe',
          ],
          'action': [
            'cp', '<(PRODUCT_DIR)/pagespeed.nexe',
            '<(PRODUCT_DIR)/pagespeed/pagespeed_<(target_arch).nexe',
          ],
        },
      ],
    },
  ],
}
