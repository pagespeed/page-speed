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
      'target_name': 'pagespeed_l10n',
      'type': '<(library)',
      'dependencies': [
        '<(DEPTH)/base/base.gyp:base',
        '<(pagespeed_root)/pagespeed/core/core.gyp:pagespeed_core',
        '<(pagespeed_root)/pagespeed/formatters/formatter_util.gyp:pagespeed_formatter_util',
        '<(pagespeed_root)/pagespeed/po/po_gen.gyp:pagespeed_genpo',
      ],
      'sources': [
        'localizer.cc',
        'gettext_localizer.cc',
        'register_locale.cc',
      ],
      'include_dirs': [
        '<(pagespeed_root)',
      ],
      'direct_dependent_settings': {
        'include_dirs': [
          '<(pagespeed_root)',
        ],
      },
      'export_dependent_settings': [
        '<(DEPTH)/base/base.gyp:base',
        '<(pagespeed_root)/pagespeed/formatters/formatter_util.gyp:pagespeed_formatter_util',
        '<(pagespeed_root)/pagespeed/po/po_gen.gyp:pagespeed_genpo',
      ]
    },
  ],
}
