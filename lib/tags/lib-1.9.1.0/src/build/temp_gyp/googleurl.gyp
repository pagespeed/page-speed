# Copyright (c) 2009 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# Slightly modified version of googleurl library target with a custom
# implementation of url_canon_icu.cc to break a dependency on icu.
{
  'variables': {
    'chromium_code': 1,
  },
  'targets': [
    {
      'target_name': 'googleurl',
      'type': '<(library)',
      'dependencies': [
        '<(DEPTH)/base/base.gyp:base',
      ],
      'sources': [
        '<(DEPTH)/googleurl_noicu/gurl.cc',
        '<(DEPTH)/googleurl/src/gurl.h',
        '<(DEPTH)/googleurl/src/url_canon.h',
        '<(DEPTH)/googleurl/src/url_canon_etc.cc',
        '<(DEPTH)/googleurl/src/url_canon_fileurl.cc',
        '<(DEPTH)/googleurl/src/url_canon_host.cc',
        '<(DEPTH)/googleurl_noicu/url_canon_noicu.cc',
        '<(DEPTH)/googleurl/src/url_canon_icu.h',
        '<(DEPTH)/googleurl/src/url_canon_internal.cc',
        '<(DEPTH)/googleurl/src/url_canon_internal.h',
        '<(DEPTH)/googleurl/src/url_canon_internal_file.h',
        '<(DEPTH)/googleurl/src/url_canon_ip.cc',
        '<(DEPTH)/googleurl/src/url_canon_ip.h',
        '<(DEPTH)/googleurl/src/url_canon_mailtourl.cc',
        '<(DEPTH)/googleurl/src/url_canon_path.cc',
        '<(DEPTH)/googleurl/src/url_canon_pathurl.cc',
        '<(DEPTH)/googleurl/src/url_canon_query.cc',
        '<(DEPTH)/googleurl/src/url_canon_relative.cc',
        '<(DEPTH)/googleurl/src/url_canon_stdstring.h',
        '<(DEPTH)/googleurl/src/url_canon_stdurl.cc',
        '<(DEPTH)/googleurl/src/url_file.h',
        '<(DEPTH)/googleurl/src/url_parse.cc',
        '<(DEPTH)/googleurl/src/url_parse.h',
        '<(DEPTH)/googleurl/src/url_parse_file.cc',
        '<(DEPTH)/googleurl/src/url_parse_internal.h',
        '<(DEPTH)/googleurl/src/url_util.cc',
        '<(DEPTH)/googleurl/src/url_util.h',
      ],
      'include_dirs': [
        '<(DEPTH)',
      ],
      'direct_dependent_settings': {
        'include_dirs': [
          '<(DEPTH)',
        ],
      },
    },
  ],
}
