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

# Defines build rules for l10n data files (.po files).
# To add a set of translations string foo.po, you must add it to the 
# prod_locales variable below.

{
  'variables': {
    'pagespeed_root': '../..',
    'chromium_code': 1,
    'poc_out_dir': '<(SHARED_INTERMEDIATE_DIR)/poc_out',
    'poc_executable': '<(pagespeed_root)/build/poc/poc',
    'poc_gyp_helper': '<(pagespeed_root)/build/poc/poc_gyp_helper',
    # List of all locales to include in "production" builds.  These locales
    # will be linked into any target depending on the pagespeed_all_po target
    # below.
    'prod_locales': [
      'ar',
      'bg',
      'ca',
      'cs',
      'da',
      'de',
      'de_AT',
      'de_CH',
      'el',
      'en_GB',
      'en_IE',
      'en_IN',
      'en_SG',
      'en_ZA',
      'es',
      'fi',
      'fil',
      'fr',
      'fr_CH',
      'gsw',
      'he',
      'hi',
      'hr',
      'hu',
      'id',
      'in',
      'it',
      'iw',
      'ja',
      'ko',
      'ln',
      'lt',
      'lv',
      'mo',
      'nl',
      'no',
      'pl',
      'pt',
      'pt_BR',
      'pt_PT',
      'ro',
      'ru',
      'sk',
      'sl',
      'sr',
      'sv',
      'th',
      'tl',
      'tr',
      'uk',
      'vi',
      'zh',
      'zh_CN',
      'zh_HK',
      'zh_TW',
    ],

    # Locales/translations used by test cases.  These locales will be linked 
    # into all targets depending on the pagespeed_test_po target below.
    'test_locales': [
      'test',
      'test_empty',
      'test_encoding',
    ],
  },
  'targets': [
    {
      # target to generate .po.cc files from .po files.  For each .po file in
      # 'sources', it runs build/poc/poc which generates a .po.cc file.
      # Also generates master.po.cc from pagespeed.pot.
      'target_name': 'pagespeed_genpo',
      'type': 'none',
      'hard_dependency': 1,
      'sources': [
        'pagespeed.pot',
        # Use poc_gyp_helper to generate .po paths from locale names, e.g.
        #   zh_CN --> zh_CN.po
        '<!@(<(poc_gyp_helper) po_files "<@(prod_locales)")',
        '<!@(<(poc_gyp_helper) po_files "<@(test_locales)")',
      ],
      'rules': [
        {
          'rule_name': 'genpo',
          'extension': 'po',
          'inputs': [
            '<(poc_executable)',
            '<(pagespeed_root)/pagespeed/po/pagespeed.pot',
          ],
          'outputs': [
            '<(poc_out_dir)/pagespeed/po/<(RULE_INPUT_ROOT).po.cc',
          ],
          'action': [
            'python',
            '<(poc_executable)',
            '<(poc_out_dir)/pagespeed/po',
            '<(pagespeed_root)/pagespeed/po/pagespeed.pot',
            '<(RULE_INPUT_PATH)',
          ],
          'message': 'Generating C code from <(RULE_INPUT_PATH)',
        },
        {
          'rule_name': 'genmasterpo',
          'extension': 'pot',
          'inputs': [
            '<(poc_executable)',
            '<(pagespeed_root)/pagespeed/po/pagespeed.pot',
          ],
          'outputs': [
            '<(poc_out_dir)/pagespeed/po/master.po.cc',
          ],
          'action': [
            'python',
            '<(poc_executable)',
            '<(poc_out_dir)/pagespeed/po',
            '<(pagespeed_root)/pagespeed/po/pagespeed.pot',
          ],
          'message': 'Generating C code from <(RULE_INPUT_PATH)',
        },
      ],
      'direct_dependent_settings': {
        'include_dirs': [
          '<(pagespeed_root)',
        ],
      },
    },
    {
      # target to include .po files in binaries and libraries.  We can't use a
      # '<(library)' target, because there are no external dependencies on the
      # .po.cc files (they register themselves dynamically).  If we link like a
      # normal library, then the linker will strip out our object files since
      # none of their symbols are referenced elsewhere in the program.  Instead,
      # we export the "sources" property, adding "foo.po.cc" to the "sources"
      # list of any target that depends on this one, forcing it to get compiled
      # and linked in.
      #
      # Note that this makes depending on this target a little fragile.  In
      # particular, a given binary/library should only depend on this target
      # once (in other words, only top-level binaries and libraries should
      # depend on it).
      'target_name': 'pagespeed_all_po',
      'type': 'none',
      'hard_dependency': 1,
      'direct_dependent_settings': {
        'sources': [
          # Use poc_gyp_helper to generate .cc paths from locale names, e.g.
          #   zh_CN  -->  <(poc_out_dir)/pagespeed/po/zh_CN.po.cc
          '<!@(<(poc_gyp_helper) cc_files \'<(poc_out_dir)/pagespeed/po\' "<@(prod_locales)")',
          '<(poc_out_dir)/pagespeed/po/master.po.cc',
        ],
      },
      'dependencies': [
        'pagespeed_genpo',
      ],
      'export_dependent_settings': [
        'pagespeed_genpo',
      ],
    },
    {
      'target_name': 'pagespeed_test_po',
      'type': 'none',
      'hard_dependency': 1,
      'direct_dependent_settings': {
        'sources': [
          # Use poc_gyp_helper to generate .cc paths from locale names, e.g.
          #   zh_CN  -->  <(poc_out_dir)/pagespeed/po/zh_CN.po.cc
          '<!@(<(poc_gyp_helper) cc_files \'<(poc_out_dir)/pagespeed/po\' "<@(test_locales)")',
          '<(poc_out_dir)/pagespeed/po/master.po.cc',
        ],
      },
      'dependencies': [
        'pagespeed_genpo',
      ],
      'export_dependent_settings': [
        'pagespeed_genpo',
      ],
    },
  ],
}
