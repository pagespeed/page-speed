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
      'target_name': 'pagespeed_html',
      'type': '<(library)',
      'dependencies': [
        '<(DEPTH)/base/base.gyp:base',
        '<(DEPTH)/<(instaweb_src_root)/instaweb_core.gyp:instaweb_htmlparse_core',
        '<(DEPTH)/<(instaweb_src_root)/instaweb_core.gyp:instaweb_rewriter_html',
        '<(pagespeed_root)/pagespeed/css/css.gyp:pagespeed_cssmin',
        '<(pagespeed_root)/pagespeed/js/js.gyp:pagespeed_jsminify',
      ],
      'sources': [
        'html_minifier.cc',
        'minify_js_css_filter.cc',
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
        '<(DEPTH)/<(instaweb_src_root)/instaweb_core.gyp:instaweb_htmlparse_core',
        '<(DEPTH)/<(instaweb_src_root)/instaweb_core.gyp:instaweb_rewriter_html',
      ],
    },
    {
      'target_name': 'pagespeed_external_resource_filter',
      'type': '<(library)',
      'dependencies': [
        '<(DEPTH)/base/base.gyp:base',
        '<(DEPTH)/<(instaweb_src_root)/instaweb_core.gyp:instaweb_htmlparse_core',
        '<(DEPTH)/<(instaweb_src_root)/instaweb_core.gyp:instaweb_rewriter_html',
      ],
      'sources': [
        'external_resource_filter.cc',
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
        '<(DEPTH)/<(instaweb_src_root)/instaweb_core.gyp:instaweb_htmlparse_core',
        '<(DEPTH)/<(instaweb_src_root)/instaweb_core.gyp:instaweb_rewriter_html',
      ],
    },
  ],
}
