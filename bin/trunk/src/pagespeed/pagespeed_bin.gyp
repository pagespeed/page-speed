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
  'includes': [
    '../third_party/WebKit/WebKitTools/DumpRenderTree/DumpRenderTree.gypi',
  ],
  'variables': {
    'chromium_src_dir': '<(DEPTH)',
    'ahem_path': '<(DEPTH)/third_party/WebKit/WebKitTools/DumpRenderTree/qt/fonts/AHEM____.TTF',
  },
  'targets': [
    {
      'target_name': 'pagespeed_chromium_lib',
      'type': '<(library)',
      'dependencies': [
        'drtlib',
        '<(DEPTH)/third_party/icu/icu.gyp:icuuc',
        '<(DEPTH)/third_party/libpagespeed/src/pagespeed/core/core.gyp:pagespeed_core',
        '<(DEPTH)/third_party/WebKit/WebKit/chromium/WebKit.gyp:webkit',
      ],
      'include_dirs': [
        '<(DEPTH)',
      ],
      'sources': [
        'http_content_decoder.cc',
        'pagespeed_input_populator.cc',
        'test_shell_runner.cc',
      ],
    },
    {
      'target_name': 'pagespeed_chromium',
      'type': 'executable',
      'dependencies': [
        '<(DEPTH)/base/base.gyp:base',
        'pagespeed_chromium_lib',
        '<(DEPTH)/third_party/libpagespeed/src/pagespeed/pagespeed.gyp:pagespeed',
        '<(DEPTH)/third_party/libpagespeed/src/pagespeed/formatters/formatters.gyp:pagespeed_formatters',
        '<(DEPTH)/third_party/libpagespeed/src/pagespeed/image_compression/image_compression.gyp:pagespeed_image_attributes_factory',
      ],
      'include_dirs': [
        '<(DEPTH)',
      ],
      'sources': [
        'pagespeed.cc',
      ],
    },
    {
      # TODO: push this upstream to WebKit.gyp
      'target_name': 'drtlib',
      'type': '<(library)',
      'dependencies': [
        '<(DEPTH)/third_party/WebKit/WebKit/chromium/WebKit.gyp:webkit',
        '<(DEPTH)/third_party/WebKit/JavaScriptCore/JavaScriptCore.gyp/JavaScriptCore.gyp:wtf_config',
        '<(chromium_src_dir)/third_party/icu/icu.gyp:icuuc',
        '<(chromium_src_dir)/webkit/support/webkit_support.gyp:npapi_layout_test_plugin',
        '<(chromium_src_dir)/webkit/support/webkit_support.gyp:webkit_support',
        '<(chromium_src_dir)/gpu/gpu.gyp:gles2_c_lib'
      ],
      'include_dirs': [
        '<(DEPTH)/third_party/WebKit/WebKit/chromium',
        '<(DEPTH)/third_party/WebKit/JavaScriptCore',
        '<(DEPTH)/third_party/WebKit/JavaScriptCore/wtf', # wtf/text/*.h refers headers in wtf/ without wtf/.
        '<(DEPTH)',
      ],
      'direct_dependent_settings': {
        'include_dirs': [
          '<(DEPTH)/third_party/WebKit/WebKit/chromium',
          '<(DEPTH)/third_party/WebKit/JavaScriptCore',
          '<(DEPTH)/third_party/WebKit/JavaScriptCore/wtf', # wtf/text/*.h refers headers in wtf/ without wtf/.
          '<(DEPTH)',
        ],
      },
      'defines': [
        # Technically not a unit test but require functions available only to
        # unit tests.
        'UNIT_TEST',
      ],
      'sources': [
        '<@(drt_files)',
      ],
      'sources/': [
        ['exclude', 'DumpRenderTree.cpp'],
      ],
      'conditions': [
        ['OS=="win"', {
          'dependencies': [
            '<(DEPTH)/third_party/WebKit/WebKit/chromium:LayoutTestHelper'
          ],
          'resource_include_dirs': ['<(SHARED_INTERMEDIATE_DIR)/webkit'],
          'sources': [
            '<(SHARED_INTERMEDIATE_DIR)/net/net_resources.rc',
            '<(SHARED_INTERMEDIATE_DIR)/webkit/webkit_chromium_resources.rc',
            '<(SHARED_INTERMEDIATE_DIR)/webkit/webkit_resources.rc',
            '<(SHARED_INTERMEDIATE_DIR)/webkit/webkit_strings_en-US.rc',
          ],
          'copies': [{
            'destination': '<(PRODUCT_DIR)',
            'files': ['<(ahem_path)'],
          }, {
            # This should really be done in the 'npapi_layout_test_plugin'
            # target, but the current VS generator handles 'copies'
            # settings as AdditionalDependencies, which means that
            # when it's over there, it tries to do the copy *before*
            # the file is built, instead of after.  We work around this
            # by attaching the copy here, since it depends on that
            # target.
            'destination': '<(PRODUCT_DIR)/plugins',
            'files': ['<(PRODUCT_DIR)/npapi_layout_test_plugin.dll'],
          }],
        },{ # OS!="win"
          'sources/': [
            ['exclude', 'Win\\.cpp$'],
          ],
          'actions': [
            {
              'action_name': 'repack_locale',
              'variables': {
                'repack_path': '<(chromium_src_dir)/tools/data_pack/repack.py',
                'pak_inputs': [
                  '<(SHARED_INTERMEDIATE_DIR)/net/net_resources.pak',
                  '<(SHARED_INTERMEDIATE_DIR)/webkit/webkit_chromium_resources.pak',
                  '<(SHARED_INTERMEDIATE_DIR)/webkit/webkit_strings_en-US.pak',
                  '<(SHARED_INTERMEDIATE_DIR)/webkit/webkit_resources.pak',
              ]},
              'inputs': [
                '<(repack_path)',
                '<@(pak_inputs)',
              ],
              'outputs': [
                '<(INTERMEDIATE_DIR)/repack/DumpRenderTree.pak',
              ],
              'action': ['python', '<(repack_path)', '<@(_outputs)', '<@(pak_inputs)'],
              'process_outputs_as_mac_bundle_resources': 1,
            },
          ], # actions
        }],
        ['OS=="mac"', {
          'dependencies': [
            '<(DEPTH)/third_party/WebKit/WebKit/chromium:LayoutTestHelper'
          ],
          'mac_bundle_resources': [
            '<(ahem_path)',
            '<(DEPTH)/third_party/WebKit/WebKitTools/DumpRenderTree/fonts/WebKitWeightWatcher100.ttf',
            '<(DEPTH)/third_party/WebKit/WebKitTools/DumpRenderTree/fonts/WebKitWeightWatcher200.ttf',
            '<(DEPTH)/third_party/WebKit/WebKitTools/DumpRenderTree/fonts/WebKitWeightWatcher300.ttf',
            '<(DEPTH)/third_party/WebKit/WebKitTools/DumpRenderTree/fonts/WebKitWeightWatcher400.ttf',
            '<(DEPTH)/third_party/WebKit/WebKitTools/DumpRenderTree/fonts/WebKitWeightWatcher500.ttf',
            '<(DEPTH)/third_party/WebKit/WebKitTools/DumpRenderTree/fonts/WebKitWeightWatcher600.ttf',
            '<(DEPTH)/third_party/WebKit/WebKitTools/DumpRenderTree/fonts/WebKitWeightWatcher700.ttf',
            '<(DEPTH)/third_party/WebKit/WebKitTools/DumpRenderTree/fonts/WebKitWeightWatcher800.ttf',
            '<(DEPTH)/third_party/WebKit/WebKitTools/DumpRenderTree/fonts/WebKitWeightWatcher900.ttf',
            '<(SHARED_INTERMEDIATE_DIR)/webkit/textAreaResizeCorner.png',
          ],
          'copies': [{
            'destination': '<(PRODUCT_DIR)/DumpRenderTree.app/Contents/PlugIns/',
            'files': ['<(PRODUCT_DIR)/TestNetscapePlugIn.plugin/'],
          }],
        },{ # OS!="mac"
          'sources/': [
            # .mm is already excluded by common.gypi
            ['exclude', 'Mac\\.cpp$'],
          ]
        }],
        ['OS=="linux" or OS=="freebsd" or OS=="openbsd" or OS=="solaris"', {
          'copies': [{
            'destination': '<(PRODUCT_DIR)',
            'files': [
              '<(ahem_path)',
              '<(DEPTH)/third_party/WebKit/WebKitTools/DumpRenderTree/chromium/fonts.conf',
              '<(INTERMEDIATE_DIR)/repack/DumpRenderTree.pak',
            ]
          }, {
            'destination': '<(PRODUCT_DIR)/plugins',
            'files': ['<(PRODUCT_DIR)/libnpapi_layout_test_plugin.so'],
          }],
        },{ # OS!="linux" and OS!="freebsd" and OS!="openbsd" and OS!="solaris"
          'sources/': [
            ['exclude', '(Gtk|Linux)\\.cpp$']
          ]
        }],
      ]
    }
  ],
}
