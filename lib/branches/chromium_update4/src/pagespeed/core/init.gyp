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
    'pagespeed_root': '../..',
    'chromium_code': 1,
  },
  'targets': [
    {
      'target_name': 'pagespeed_init',
      'type': '<(library)',
      'dependencies': [
        '<(DEPTH)/base/base.gyp:base',
        '<(DEPTH)/build/temp_gyp/googleurl.gyp:googleurl',
        '<(DEPTH)/third_party/domain_registry_provider/src/domain_registry/domain_registry.gyp:init_registry_tables_lib',
        '<(DEPTH)/<(instaweb_src_root)/instaweb_core.gyp:instaweb_htmlparse_core',
        '<(pagespeed_root)/pagespeed/l10n/l10n.gyp:pagespeed_l10n',
      ],
      'sources': [
        'cpu_compatibility.cc',
        'pagespeed_init.cc',
      ],
    },
  ],
}
