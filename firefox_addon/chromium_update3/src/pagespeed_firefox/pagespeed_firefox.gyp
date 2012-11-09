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
    'libpagespeed_root': '<(DEPTH)/third_party/libpagespeed/src',
    'xulrunner_sdk_root': '<(DEPTH)/third_party/xulrunner-sdk/<(xulrunner_sdk_version)',
    'xulrunner_sdk_os_root': '<(xulrunner_sdk_root)/arch/<(OS)',
    'xulrunner_sdk_arch_root': '<(xulrunner_sdk_os_root)/<(host_arch)',
    'xpidl_out_dir': '<(SHARED_INTERMEDIATE_DIR)/xpidl_out',
  },
  'targets': [
    {
      'target_name': 'pagespeed_firefox_genxpt',
      'type': 'none',
      'variables': {
        'xpt_output_path': '<(xpidl_out_dir)/pagespeed_firefox/xpi_resources/components',
      },
      'sources': [
        'idl/IComponentCollector.idl',
        'idl/IStateStorage.idl',
      ],
      'rules': [
        {
          'rule_name': 'genxpt',
          'extension': 'idl',
          'variables': {
            'rule_input_relpath': 'idl',
          },
          'outputs': [
            '<(xpt_output_path)/<(RULE_INPUT_ROOT).xpt',
          ],
          'action': [
            '<(xulrunner_sdk_arch_root)/bin/xpidl',
            '-m', 'typelib',
            '-w',
            '-I', '<(xulrunner_sdk_root)/idl',
            '-e', '<(xpt_output_path)/<(RULE_INPUT_ROOT).xpt',
            './<(rule_input_relpath)/<(RULE_INPUT_ROOT)<(RULE_INPUT_EXT)',
          ],
          'message': 'Generating xpt components from <(RULE_INPUT_PATH)',
        },
      ],
      'dependencies': [
        '<(DEPTH)/third_party/xulrunner-sdk/xulrunner.gyp:xulrunner_sdk',
      ],
    },
    {
      'target_name': 'pagespeed_firefox_json_input',
      'type': '<(library)',
      'dependencies': [
        '<(DEPTH)/base/base.gyp:base',
        '<(libpagespeed_root)/pagespeed/core/core.gyp:pagespeed_core',
      ],
      'sources': [
        'cpp/pagespeed/pagespeed_json_input.cc',
      ],
      'export_dependent_settings': [
        '<(DEPTH)/base/base.gyp:base',
        '<(libpagespeed_root)/pagespeed/core/core.gyp:pagespeed_core',
      ],
    },
    {
      'target_name': 'pagespeed_firefox_test',
      'type': 'executable',
      'dependencies': [
        'pagespeed_firefox_json_input',
        '<(DEPTH)/testing/gtest.gyp:gtest',
        '<(DEPTH)/testing/gtest.gyp:gtest_main',
      ],
      'sources': [
        'cpp/pagespeed/pagespeed_json_input_test.cc',
      ],
    },
    {
      'target_name': 'pagespeed_firefox_module',
      'type': 'loadable_module',
      'product_name': 'pagespeed',
      'dependencies': [
        'pagespeed_firefox_json_input',
        '<(DEPTH)/base/base.gyp:base',
        '<(DEPTH)/build/temp_gyp/googleurl.gyp:googleurl',
        '<(libpagespeed_root)/pagespeed/core/core.gyp:pagespeed_core',
        '<(libpagespeed_root)/pagespeed/core/init.gyp:pagespeed_init',
        '<(libpagespeed_root)/pagespeed/dom/dom.gyp:pagespeed_json_dom',
        '<(libpagespeed_root)/pagespeed/pagespeed.gyp:pagespeed_library',
        '<(libpagespeed_root)/pagespeed/filters/filters.gyp:pagespeed_filters',
        '<(libpagespeed_root)/pagespeed/har/har.gyp:pagespeed_har',
        '<(libpagespeed_root)/pagespeed/formatters/formatters.gyp:pagespeed_formatters',
        '<(libpagespeed_root)/pagespeed/image_compression/image_compression.gyp:pagespeed_image_attributes_factory',
        '<(libpagespeed_root)/pagespeed/po/po_gen.gyp:pagespeed_all_po',
        '<(libpagespeed_root)/pagespeed/proto/proto_gen.gyp:pagespeed_output_pb',
        '<(libpagespeed_root)/pagespeed/proto/proto_gen.gyp:pagespeed_proto_formatted_results_converter',
        '<(libpagespeed_root)/pagespeed/proto/proto_gen.gyp:pagespeed_proto_results_converter',
      ],
      'sources': [
        'cpp/pagespeed/pagespeed_rules.cc',
      ],
      'msvs_settings': {
        'VCLinkerTool': {
	  'ModuleDefinitionFile': 'cpp/pagespeed/pagespeed.def',
	},
      },
    },
    {
      # Copies the pagespeed shared object to the appropriate location
      # in the source directory for archiving.
      'target_name': 'pagespeed_firefox_module_archive',
      'suppress_wildcard': 1,
      'type': 'none',
      'dependencies': [
        'pagespeed_firefox_module',
      ],
      'copies': [
        {
          'destination': '<(DEPTH)/pagespeed_firefox/xpi_resources/platform/<(xpcom_os)_<(xpcom_cpu_arch)-<(xpcom_compiler_abi)/components',
          'conditions': [
            ['OS=="mac"', {
              # On Mac, SHARED_LIB_PREFIX/SUFFIX refer to 'lib' and '.dylib'
              # but these do not match our shared library, so we manually
              # override here.
              'files': [
                '<(PRODUCT_DIR)/pagespeed.so',
              ],
            }, {
              'files': [
                '<(PRODUCT_DIR)/<(SHARED_LIB_PREFIX)pagespeed<(SHARED_LIB_SUFFIX)',
              ],
            }],
          ],
        },
      ],
    },
  ],
}
