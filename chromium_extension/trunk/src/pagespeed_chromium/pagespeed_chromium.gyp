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
    # TODO(mdsteele): Use this build target instead, once we transition back to
    #                 NaCl from NPAPI.
    # {
    #   'target_name': 'pagespeed.nexe',
    #   'type': 'executable',
    #   'dependencies': [
    #     '<(DEPTH)/base/base.gyp:base',
    #     '<(libpagespeed_root)/pagespeed/core/core.gyp:pagespeed_core',
    #     '<(libpagespeed_root)/pagespeed/pagespeed.gyp:pagespeed_library',
    #     '<(libpagespeed_root)/pagespeed/filters/filters.gyp:pagespeed_filters',
    #     '<(libpagespeed_root)/pagespeed/formatters/formatters.gyp:pagespeed_formatters',
    #     '<(libpagespeed_root)/pagespeed/har/har.gyp:pagespeed_har',
    #     '<(libpagespeed_root)/pagespeed/image_compression/image_compression.gyp:pagespeed_image_attributes_factory',
    #   ],
    #   'sources': [
    #     'npapi_dom.cc',
    #     'npp_gate.cc',
    #     'pagespeed_chromium.cc',
    #     'pagespeed_module.cc',
    #   ],
    #   'ldflags': [
    #     '-lgoogle_nacl_imc',
    #     '-lgoogle_nacl_npruntime',
    #     '-lpthread',
    #     '-lsrpc',
    #   ],
    # },
    {
      'target_name': 'pagespeed_plugin',
      'type': 'loadable_module',
      'dependencies': [
        '<(DEPTH)/base/base.gyp:base',
        '<(libpagespeed_root)/pagespeed/core/core.gyp:pagespeed_core',
        '<(libpagespeed_root)/pagespeed/pagespeed.gyp:pagespeed_library',
        '<(libpagespeed_root)/pagespeed/filters/filters.gyp:pagespeed_filters',
        '<(libpagespeed_root)/pagespeed/formatters/formatters.gyp:pagespeed_formatters',
        '<(libpagespeed_root)/pagespeed/har/har.gyp:pagespeed_har',
        '<(libpagespeed_root)/pagespeed/image_compression/image_compression.gyp:pagespeed_image_attributes_factory',
        '<(libpagespeed_root)/pagespeed/proto/proto_gen.gyp:pagespeed_output_pb',
        '<(libpagespeed_root)/pagespeed/proto/proto_gen.gyp:pagespeed_proto_formatted_results_converter',
      ],
      'sources': [
        'pagespeed_chromium.cc',
        'npapi_dom.cc',
        'npapi/np_entry.cc',
        'npapi/npp_entry.cc',
      ],
      'include_dirs': [
        '<(DEPTH)',
      ],
      'msvs_settings': {
        'VCLinkerTool': {
	  'ModuleDefinitionFile': 'pagespeed_plugin.def',
	},
      },
    },
    {
      'target_name': 'pagespeed_extension',
      'type': 'none',
      'dependencies': [
        'pagespeed_plugin',
        # 'pagespeed.nexe',
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
            'extension_files/audits.js',
            'extension_files/background.html',
            'extension_files/background.js',
            'extension_files/content-script.js',
            'extension_files/devtools-page.html',
            'extension_files/errorRedDot.png',
            'extension_files/manifest.json',
            'extension_files/pagespeed-32.png',
            'extension_files/pagespeed-64.png',
            'extension_files/pagespeed-128.png',
            'extension_files/pagespeed.js',
            'extension_files/pagespeed-panel.css',
            'extension_files/pagespeed-panel.html',
            'extension_files/spinner.gif',
            'extension_files/successGreenDot.png',
            'extension_files/warningOrangeDot.png',
          ],
          'outputs': [
            '<(PRODUCT_DIR)/pagespeed/audits.js',
            '<(PRODUCT_DIR)/pagespeed/background.html',
            '<(PRODUCT_DIR)/pagespeed/background.js',
            '<(PRODUCT_DIR)/pagespeed/content-script.js',
            '<(PRODUCT_DIR)/pagespeed/devtools-page.html',
            '<(PRODUCT_DIR)/pagespeed/errorRedDot.png',
            '<(PRODUCT_DIR)/pagespeed/manifest.json',
            '<(PRODUCT_DIR)/pagespeed/pagespeed-32.png',
            '<(PRODUCT_DIR)/pagespeed/pagespeed-64.png',
            '<(PRODUCT_DIR)/pagespeed/pagespeed-128.png',
            '<(PRODUCT_DIR)/pagespeed/pagespeed.js',
            '<(PRODUCT_DIR)/pagespeed/pagespeed-panel.css',
            '<(PRODUCT_DIR)/pagespeed/pagespeed-panel.html',
            '<(PRODUCT_DIR)/pagespeed/spinner.gif',
            '<(PRODUCT_DIR)/pagespeed/successGreenDot.png',
            '<(PRODUCT_DIR)/pagespeed/warningOrangeDot.png',
          ],
          'action': [
            'cp', '-t', '<@(_inputs)',
          ],
        },
        # TODO(mdsteele): Use this build target instead, once we transition
        #                 back to NaCl from NPAPI.
        # {
        #   'action_name': 'copy_nexe',
        #   'inputs': [
        #     '<(PRODUCT_DIR)/pagespeed',
        #     '<(PRODUCT_DIR)/pagespeed.nexe',
        #   ],
        #   'outputs': [
        #     '<(PRODUCT_DIR)/pagespeed/pagespeed_<(target_arch).nexe',
        #   ],
        #   'action': [
        #     'cp', '<(PRODUCT_DIR)/pagespeed.nexe',
        #     '<(PRODUCT_DIR)/pagespeed/pagespeed_<(target_arch).nexe',
        #   ],
        # },
        {
          'action_name': 'copy_so',
          'variables': {
            'input_path': '<(PRODUCT_DIR)/<(SHARED_LIB_PREFIX)pagespeed_plugin<(SHARED_LIB_SUFFIX)',
            'conditions': [
              ['OS=="win"', {
                'os': 'WINNT',
                'compiler_abi': 'msvc',
              }],
              ['OS=="linux"', {
                'os': 'Linux',
                'compiler_abi': 'gcc3',
              }],
              ['OS=="mac"', {
                'os': 'Darwin',
                'compiler_abi': 'gcc3',
              }],
              ['target_arch=="ia32"', {
                'cpu_arch': 'x86',
              }],
              ['target_arch=="x64"', {
                'cpu_arch': 'x86_64',
              }],
            ],
          },
          'inputs': [
            '<(PRODUCT_DIR)/pagespeed',
            '<(input_path)',
          ],
          'outputs': [
            '<(PRODUCT_DIR)/pagespeed/<(SHARED_LIB_PREFIX)pagespeed_plugin_<(os)_<(cpu_arch)-<(compiler_abi)<(SHARED_LIB_SUFFIX)',
          ],
          'action': [
            'cp', '<(input_path)',
            '<(PRODUCT_DIR)/pagespeed/<(SHARED_LIB_PREFIX)pagespeed_plugin_<(os)_<(cpu_arch)-<(compiler_abi)<(SHARED_LIB_SUFFIX)',
          ],
        },
      ],
    },
  ],
}
