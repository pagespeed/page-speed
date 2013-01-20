# Copyright (c) 2009 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

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
        '<(DEPTH)/third_party/icu/icu.gyp:icuuc',
      ],
      'sources': [
        '<(DEPTH)/googleurl/src/gurl.cc',
        '<(DEPTH)/googleurl/src/gurl.h',
        '<(DEPTH)/googleurl/src/url_canon.h',
        '<(DEPTH)/googleurl/src/url_canon_etc.cc',
        '<(DEPTH)/googleurl/src/url_canon_fileurl.cc',
        '<(DEPTH)/googleurl/src/url_canon_host.cc',
        '<(DEPTH)/googleurl/src/url_canon_icu.cc',
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
      # TODO(jschuh): crbug.com/167187 fix size_t to int truncations.
      'msvs_disabled_warnings': [4267, ],
    },
    {
      'target_name': 'googleurl_unittests',
      'type': 'executable',
      'dependencies': [
        'googleurl',
        '<(DEPTH)/base/base.gyp:base',
        '<(DEPTH)/testing/gtest.gyp:gtest',
        '<(DEPTH)/third_party/icu/icu.gyp:icuuc',
      ],
      'defines': [
        # Our ICU build does not provide character set converters. We
        # set this define to disable tests that depend on non-default
        # character set conversions.
        'ICU_NO_CONVERTER_DATA',
      ],
      'sources': [
        '<(DEPTH)/googleurl/src/gurl_unittest.cc',
        '<(DEPTH)/googleurl_noconv/src/url_canon_unittest.cc',
        '<(DEPTH)/googleurl/src/url_parse_unittest.cc',
        '<(DEPTH)/googleurl/src/url_test_utils.h',
        '<(DEPTH)/googleurl/src/url_util_unittest.cc',
        '<(DEPTH)/googleurl/src/gurl_test_main.cc',
      ],
      # TODO(jschuh): crbug.com/167187 fix size_t to int truncations.
      'msvs_disabled_warnings': [4267, ],
    },
  ],
}

# Local Variables:
# tab-width:2
# indent-tabs-mode:nil
# End:
# vim: set expandtab tabstop=2 shiftwidth=2:
