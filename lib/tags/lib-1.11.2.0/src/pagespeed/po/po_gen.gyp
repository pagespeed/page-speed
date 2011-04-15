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
# To add a set of translations string foo.po, you must do two things:
#   1) add foo.po to pagespeed_genpo['sources'] --- this turns foo.po
#      into foo.po.cc during the build process
#   2) add foo.po.cc to pagespeed_all_po['direct_dependent_settings']['sources']
#      (or another target that exposes a collection of .po files) --- this links
#      the generated foo.po.cc file in with any targets that depend on
#      pagespeed_all_po by exporting the "sources" directive (with foo.po.cc) up
#      the dependency tree.

{
  'variables': {
    'pagespeed_root': '../..',
    'chromium_code': 1,
    'poc_out_dir': '<(SHARED_INTERMEDIATE_DIR)/poc_out',
    'poc_executable': '<(pagespeed_root)/build/poc/poc',
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
        'ar.po',
        'bg.po',
        'ca.po',
        'cs.po',
        'da.po',
        'de.po',
        'de_AT.po',
        'de_CH.po',
        'el.po',
        'en_GB.po',
        'en_IE.po',
        'en_IN.po',
        'en_SG.po',
        'en_ZA.po',
        'es.po',
        'fi.po',
        'fil.po',
        'fr.po',
        'fr_CH.po',
        'gsw.po',
        'he.po',
        'hi.po',
        'hr.po',
        'hu.po',
        'id.po',
        'in.po',
        'it.po',
        'iw.po',
        'ja.po',
        'ko.po',
        'ln.po',
        'lt.po',
        'lv.po',
        'mo.po',
        'nl.po',
        'no.po',
        'pl.po',
        'pt.po',
        'pt_BR.po',
        'pt_PT.po',
        'ro.po',
        'ru.po',
        'sk.po',
        'sl.po',
        'sr.po',
        'sv.po',
        'th.po',
        'tl.po',
        'tr.po',
        'uk.po',
        'vi.po',
        'zh.po',
        'zh_CN.po',
        'zh_HK.po',
        'zh_TW.po',

        # translations used by test cases
        'test.po',
        'test_empty.po',
        'test_encoding.po',
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
          # for each desired locale, list the .po.cc file here.  To disable a
          # locale in a build, simply comment out the appropriate line.
          '<(poc_out_dir)/pagespeed/po/ar.po.cc',
          '<(poc_out_dir)/pagespeed/po/bg.po.cc',
          '<(poc_out_dir)/pagespeed/po/ca.po.cc',
          '<(poc_out_dir)/pagespeed/po/cs.po.cc',
          '<(poc_out_dir)/pagespeed/po/da.po.cc',
          '<(poc_out_dir)/pagespeed/po/de.po.cc',
          '<(poc_out_dir)/pagespeed/po/de_AT.po.cc',
          '<(poc_out_dir)/pagespeed/po/de_CH.po.cc',
          '<(poc_out_dir)/pagespeed/po/el.po.cc',
          '<(poc_out_dir)/pagespeed/po/en_GB.po.cc',
          '<(poc_out_dir)/pagespeed/po/en_IE.po.cc',
          '<(poc_out_dir)/pagespeed/po/en_IN.po.cc',
          '<(poc_out_dir)/pagespeed/po/en_SG.po.cc',
          '<(poc_out_dir)/pagespeed/po/en_ZA.po.cc',
          '<(poc_out_dir)/pagespeed/po/es.po.cc',
          '<(poc_out_dir)/pagespeed/po/fi.po.cc',
          '<(poc_out_dir)/pagespeed/po/fil.po.cc',
          '<(poc_out_dir)/pagespeed/po/fr.po.cc',
          '<(poc_out_dir)/pagespeed/po/fr_CH.po.cc',
          '<(poc_out_dir)/pagespeed/po/gsw.po.cc',
          '<(poc_out_dir)/pagespeed/po/he.po.cc',
          '<(poc_out_dir)/pagespeed/po/hi.po.cc',
          '<(poc_out_dir)/pagespeed/po/hr.po.cc',
          '<(poc_out_dir)/pagespeed/po/hu.po.cc',
          '<(poc_out_dir)/pagespeed/po/id.po.cc',
          '<(poc_out_dir)/pagespeed/po/in.po.cc',
          '<(poc_out_dir)/pagespeed/po/it.po.cc',
          '<(poc_out_dir)/pagespeed/po/iw.po.cc',
          '<(poc_out_dir)/pagespeed/po/ja.po.cc',
          '<(poc_out_dir)/pagespeed/po/ko.po.cc',
          '<(poc_out_dir)/pagespeed/po/ln.po.cc',
          '<(poc_out_dir)/pagespeed/po/lt.po.cc',
          '<(poc_out_dir)/pagespeed/po/lv.po.cc',
          '<(poc_out_dir)/pagespeed/po/mo.po.cc',
          '<(poc_out_dir)/pagespeed/po/nl.po.cc',
          '<(poc_out_dir)/pagespeed/po/no.po.cc',
          '<(poc_out_dir)/pagespeed/po/pl.po.cc',
          '<(poc_out_dir)/pagespeed/po/pt.po.cc',
          '<(poc_out_dir)/pagespeed/po/pt_BR.po.cc',
          '<(poc_out_dir)/pagespeed/po/pt_PT.po.cc',
          '<(poc_out_dir)/pagespeed/po/ro.po.cc',
          '<(poc_out_dir)/pagespeed/po/ru.po.cc',
          '<(poc_out_dir)/pagespeed/po/sk.po.cc',
          '<(poc_out_dir)/pagespeed/po/sl.po.cc',
          '<(poc_out_dir)/pagespeed/po/sr.po.cc',
          '<(poc_out_dir)/pagespeed/po/sv.po.cc',
          '<(poc_out_dir)/pagespeed/po/th.po.cc',
          '<(poc_out_dir)/pagespeed/po/tl.po.cc',
          '<(poc_out_dir)/pagespeed/po/tr.po.cc',
          '<(poc_out_dir)/pagespeed/po/uk.po.cc',
          '<(poc_out_dir)/pagespeed/po/vi.po.cc',
          '<(poc_out_dir)/pagespeed/po/zh.po.cc',
          '<(poc_out_dir)/pagespeed/po/zh_CN.po.cc',
          '<(poc_out_dir)/pagespeed/po/zh_HK.po.cc',
          '<(poc_out_dir)/pagespeed/po/zh_TW.po.cc',
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
          '<(poc_out_dir)/pagespeed/po/test.po.cc',
          '<(poc_out_dir)/pagespeed/po/test_empty.po.cc',
          '<(poc_out_dir)/pagespeed/po/test_encoding.po.cc',
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
