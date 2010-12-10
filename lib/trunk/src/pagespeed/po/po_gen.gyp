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
      # 'sources', it runs build/poc/poc which generates a .po.cc file, as
      # well as a file called 'master.po.cc' with the master strings.
      'target_name': 'pagespeed_genpo',
      'type': 'none',
      'hard_dependency': 1,
      'sources': [
        # translations used by test cases
        'backwards.po',
        'empty.po',
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
            '<(poc_out_dir)/pagespeed/po/master.po.cc',
          ],
          'action': [
            '<(poc_executable)',
            '<(poc_out_dir)/pagespeed/po',
            '<(pagespeed_root)/pagespeed/po/pagespeed.pot',
            '<(RULE_INPUT_PATH)',
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
          # for each desired locale, list the .po.cc file here
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
          '<(poc_out_dir)/pagespeed/po/backwards.po.cc',
          '<(poc_out_dir)/pagespeed/po/empty.po.cc',
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
