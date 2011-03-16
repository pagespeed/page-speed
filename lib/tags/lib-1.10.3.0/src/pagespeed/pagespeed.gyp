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
    'pagespeed_root': '..',
    'chromium_code': 1,
  },
  'targets': [
    {
      'target_name': 'pagespeed_library',
      'type': '<(library)',
      'dependencies': [
        '<(DEPTH)/base/base.gyp:base',
        '<(DEPTH)/net/net.gyp:net_base',
        '<(DEPTH)/third_party/instaweb/src/net/instaweb/instaweb_core.gyp:instaweb_htmlparse_core',
        '<(DEPTH)/third_party/zlib/zlib.gyp:zlib',
        '<(pagespeed_root)/pagespeed/core/core.gyp:pagespeed_core',
        '<(pagespeed_root)/pagespeed/cssmin/cssmin.gyp:pagespeed_cssmin',
        '<(pagespeed_root)/pagespeed/html/html.gyp:pagespeed_html',
        '<(pagespeed_root)/pagespeed/html/html.gyp:pagespeed_external_resource_filter',
        '<(pagespeed_root)/pagespeed/image_compression/image_compression.gyp:pagespeed_jpeg_optimizer',
        '<(pagespeed_root)/pagespeed/image_compression/image_compression.gyp:pagespeed_png_optimizer',
        '<(pagespeed_root)/pagespeed/jsminify/js_minify.gyp:pagespeed_jsminify',
        '<(pagespeed_root)/pagespeed/l10n/l10n.gyp:pagespeed_l10n',
        '<(pagespeed_root)/pagespeed/proto/proto_gen.gyp:pagespeed_output_pb',
      ],
      'sources': [
        'rules/avoid_bad_requests.cc',
        'rules/avoid_css_import.cc',
        'rules/avoid_document_write.cc',
        'rules/combine_external_resources.cc',
        'rules/enable_gzip_compression.cc',
        'rules/inline_small_resources.cc',
        'rules/leverage_browser_caching.cc',
        'rules/minify_css.cc',
        'rules/minify_html.cc',
        'rules/minify_javascript.cc',
        'rules/minify_rule.cc',
        'rules/minimize_dns_lookups.cc',
        'rules/minimize_redirects.cc',
        'rules/minimize_request_size.cc',
        'rules/optimize_images.cc',
        'rules/optimize_the_order_of_styles_and_scripts.cc',
        'rules/parallelize_downloads_across_hostnames.cc',
        'rules/prefer_async_resources.cc',
        'rules/put_css_in_the_document_head.cc',
        'rules/remove_query_strings_from_static_resources.cc',
        'rules/rule_provider.cc',
        'rules/savings_computer.cc',
        'rules/serve_resources_from_a_consistent_url.cc',
        'rules/serve_scaled_images.cc',
        'rules/specify_a_cache_validator.cc',
        'rules/specify_a_vary_accept_encoding_header.cc',
        'rules/specify_charset_early.cc',
        'rules/specify_image_dimensions.cc',
        'rules/sprite_images.cc',
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
        '<(pagespeed_root)/pagespeed/core/core.gyp:pagespeed_core',
        '<(pagespeed_root)/pagespeed/html/html.gyp:pagespeed_html',
        '<(pagespeed_root)/pagespeed/image_compression/image_compression.gyp:pagespeed_png_optimizer',
      ],
    },
    {
      'target_name': 'pagespeed_test',
      'type': 'executable',
      'dependencies': [
        'pagespeed_library',
        '<(pagespeed_root)/pagespeed/cssmin/cssmin.gyp:pagespeed_cssmin',
        '<(pagespeed_root)/pagespeed/filters/filters.gyp:pagespeed_filters',
        '<(pagespeed_root)/pagespeed/formatters/formatters.gyp:pagespeed_formatters',
        '<(pagespeed_root)/pagespeed/har/har.gyp:pagespeed_har',
        '<(pagespeed_root)/pagespeed/jsminify/js_minify.gyp:pagespeed_jsminify',
        '<(pagespeed_root)/pagespeed/l10n/l10n.gyp:pagespeed_l10n',
        '<(pagespeed_root)/pagespeed/po/po_gen.gyp:pagespeed_test_po',
        '<(pagespeed_root)/pagespeed/proto/proto_gen.gyp:pagespeed_input_pb',
        '<(pagespeed_root)/pagespeed/proto/proto_gen.gyp:pagespeed_output_pb',
        '<(pagespeed_root)/pagespeed/proto/proto_gen.gyp:pagespeed_proto_results_converter',
        '<(pagespeed_root)/pagespeed/proto/proto.gyp:pagespeed_proto',
        '<(pagespeed_root)/pagespeed/testing/testing.gyp:pagespeed_testing',
        '<(pagespeed_root)/pagespeed/testing/testing.gyp:pagespeed_test_main',
        '<(pagespeed_root)/pagespeed/util/util.gyp:pagespeed_util',
        '<(DEPTH)/base/base.gyp:base',
        '<(DEPTH)/testing/gtest.gyp:gtest',
      ],
      'include_dirs': [
        '<(pagespeed_root)',
      ],
      'sources': [
        'core/engine_test.cc',
        'core/input_capabilities_test.cc',
        'core/pagespeed_input_test.cc',
        'core/resource_test.cc',
        'core/resource_filter_test.cc',
        'core/resource_util_test.cc',
        'core/string_util_test.cc',
        'core/uri_util_test.cc',
        'cssmin/cssmin_test.cc',
        'filters/ad_filter_test.cc',
        'filters/protocol_filter_test.cc',
        'filters/response_byte_result_filter_test.cc',
        'filters/tracker_filter_test.cc',
        'formatters/formatter_util_test.cc',
        'formatters/json_formatter_test.cc',
        'formatters/proto_formatter_test.cc',
        'formatters/text_formatter_test.cc',
        'har/http_archive_test.cc',
        'html/external_resource_filter_test.cc',
        'html/html_minifier_test.cc',
        'jsminify/js_minify_test.cc',
        'l10n/localizer_test.cc',
        'l10n/gettext_localizer_test.cc',
        'proto/results_to_json_converter_test.cc',
        'rules/avoid_bad_requests_test.cc',
        'rules/avoid_css_import_test.cc',
        'rules/avoid_document_write_test.cc',
        'rules/combine_external_resources_test.cc',
        'rules/enable_gzip_compression_test.cc',
        'rules/inline_small_resources_test.cc',
        'rules/leverage_browser_caching_test.cc',
        'rules/minify_css_test.cc',
        'rules/minify_html_test.cc',
        'rules/minify_javascript_test.cc',
        'rules/minimize_dns_lookups_test.cc',
        'rules/minimize_redirects_test.cc',
        'rules/minimize_request_size_test.cc',
        'rules/optimize_the_order_of_styles_and_scripts_test.cc',
        'rules/parallelize_downloads_across_hostnames_test.cc',
        'rules/prefer_async_resources_test.cc',
        'rules/put_css_in_the_document_head_test.cc',
        'rules/remove_query_strings_from_static_resources_test.cc',
        'rules/rule_provider_test.cc',
        'rules/serve_resources_from_a_consistent_url_test.cc',
        'rules/serve_scaled_images_test.cc',
        'rules/specify_a_cache_validator_test.cc',
        'rules/specify_a_vary_accept_encoding_header_test.cc',
        'rules/specify_charset_early_test.cc',
        'rules/specify_image_dimensions_test.cc',
        'rules/sprite_images_test.cc',
        'testing/fake_dom_test.cc',
        'util/regex_test.cc',
      ],
    },
    {
      'target_name': 'pagespeed_image_test',
      'type': 'executable',
      'dependencies': [
        'pagespeed_library',
        '<(pagespeed_root)/pagespeed/image_compression/image_compression.gyp:pagespeed_image_attributes_factory',
        '<(pagespeed_root)/pagespeed/proto/proto_gen.gyp:pagespeed_input_pb',
        '<(pagespeed_root)/pagespeed/proto/proto_gen.gyp:pagespeed_output_pb',
        '<(pagespeed_root)/pagespeed/proto/proto.gyp:pagespeed_proto',
        '<(pagespeed_root)/pagespeed/testing/testing.gyp:pagespeed_testing',
        '<(pagespeed_root)/pagespeed/testing/testing.gyp:pagespeed_test_main',
        '<(DEPTH)/base/base.gyp:base',
        '<(DEPTH)/testing/gtest.gyp:gtest',
        '<(DEPTH)/third_party/libjpeg/libjpeg.gyp:libjpeg',
        '<(DEPTH)/third_party/libpng/libpng.gyp:libpng',
        '<(pagespeed_root)/third_party/readpng/readpng.gyp:readpng',
      ],
      'include_dirs': [
        '<(pagespeed_root)',
      ],
      'sources': [
        'image_compression/image_attributes_factory_test.cc',
        'image_compression/jpeg_optimizer_test.cc',
        'image_compression/png_optimizer_test.cc',
        'rules/optimize_images_test.cc',
      ],
      'defines': [
        'IMAGE_TEST_DIR_PATH="pagespeed/image_compression/testdata/"',
      ],
    },
  ],
}
