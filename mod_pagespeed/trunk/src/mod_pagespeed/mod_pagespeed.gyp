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
    # chromium_code indicates that the code is not
    # third-party code and should be subjected to strict compiler
    # warnings/errors in order to catch programming mistakes.
    'chromium_code': 1,
    'mod_spdy_root': '<(DEPTH)/third_party/mod_spdy/src',
  },

  'targets': [
    {
      'target_name': 'mod_pagespeed',
      'type': 'loadable_module',
      'dependencies': [
        '<(DEPTH)/base/base.gyp:base',
        '<(DEPTH)/html_rewriter/html_rewriter.gyp:html_rewriter',
        '<(DEPTH)/third_party/apache/httpd/httpd.gyp:include',
        '<(DEPTH)/third_party/instaweb/instaweb.gyp:util',
        '<(DEPTH)/third_party/libpagespeed/src/pagespeed/cssmin/cssmin.gyp:pagespeed_cssmin',
        '<(DEPTH)/third_party/libpagespeed/src/pagespeed/image_compression/image_compression.gyp:pagespeed_jpeg_optimizer',
        '<(DEPTH)/third_party/libpagespeed/src/pagespeed/image_compression/image_compression.gyp:pagespeed_png_optimizer',
        '<(DEPTH)/third_party/libpagespeed/src/third_party/jsmin/jsmin.gyp:jsmin',
        '<(DEPTH)/third_party/serf/serf.gyp:serf',
      ],
      'include_dirs': [
        '<(DEPTH)',
        '<(mod_spdy_root)',
      ],
      'sources': [
        '<(DEPTH)/mod_pagespeed/mod_pagespeed.cc',
        '<(DEPTH)/mod_pagespeed/pagespeed_process_context.cc',
        '<(mod_spdy_root)/mod_spdy/apache/log_message_handler.cc',
      ],
    },
  ],
}
