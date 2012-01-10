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
      'target_name': 'minify_html_bin',
      'type': 'executable',
      'dependencies': [
        '<(pagespeed_root)/pagespeed/core/init.gyp:pagespeed_init',
        '<(pagespeed_root)/pagespeed/html/html.gyp:pagespeed_html',
      ],
      'sources': [
        'minify_html.cc',
      ],
    },
    {
      'target_name': 'minify_css_bin',
      'type': 'executable',
      'dependencies': [
        '<(pagespeed_root)/pagespeed/core/core.gyp:pagespeed_core',
        '<(pagespeed_root)/pagespeed/core/init.gyp:pagespeed_init',
        '<(pagespeed_root)/pagespeed/css/css.gyp:pagespeed_cssmin',
      ],
      'sources': [
        'minify_css.cc',
      ],
    },
    {
      'target_name': 'minify_js_bin',
      'type': 'executable',
      'dependencies': [
        '<(pagespeed_root)/pagespeed/js/js.gyp:pagespeed_jsminify',
      ],
      'sources': [
        'minify_js.cc',
      ],
    },
    {
      'target_name': 'pagespeed_bin',
      'type': 'executable',
      'dependencies': [
        '<(DEPTH)/base/base.gyp:base',
        '<(pagespeed_root)/third_party/google-gflags/google-gflags.gyp:google-gflags',
        '<(pagespeed_root)/pagespeed/core/init.gyp:pagespeed_init',
        '<(pagespeed_root)/pagespeed/dom/dom.gyp:pagespeed_json_dom',
        '<(pagespeed_root)/pagespeed/formatters/formatters.gyp:pagespeed_formatters',
        '<(pagespeed_root)/pagespeed/har/har.gyp:pagespeed_har',
        '<(pagespeed_root)/pagespeed/image_compression/image_compression.gyp:pagespeed_image_attributes_factory',
        '<(pagespeed_root)/pagespeed/pagespeed.gyp:pagespeed_library',
        '<(pagespeed_root)/pagespeed/pdf/pdf.gyp:pagespeed_pdf',
        '<(pagespeed_root)/pagespeed/po/po_gen.gyp:pagespeed_all_po',
        '<(pagespeed_root)/pagespeed/proto/proto_gen.gyp:pagespeed_input_pb',
        '<(pagespeed_root)/pagespeed/proto/proto_gen.gyp:pagespeed_proto_formatted_results_converter',
        '<(pagespeed_root)/pagespeed/proto/proto_gen.gyp:pagespeed_proto_results_converter',
        '<(pagespeed_root)/pagespeed/proto/proto.gyp:pagespeed_proto',
        '<(pagespeed_root)/pagespeed/timeline/timeline.gyp:pagespeed_timeline',
      ],
      'sources': [
        'pagespeed.cc',
      ],
    },
    {
      'target_name': 'optimize_image_bin',
      'type': 'executable',
      'dependencies': [
        '<(pagespeed_root)/pagespeed/image_compression/image_compression.gyp:pagespeed_image_converter',
        '<(pagespeed_root)/pagespeed/image_compression/image_compression.gyp:pagespeed_jpeg_optimizer',
        '<(pagespeed_root)/pagespeed/image_compression/image_compression.gyp:pagespeed_png_optimizer',
        '<(pagespeed_root)/third_party/google-gflags/google-gflags.gyp:google-gflags',
      ],
      'sources': [
        'optimize_image.cc',
      ],
    },
    {
      'target_name': 'pagespeed_java',
      'suppress_wildcard': 1,
      'type': 'none',
      'dependencies': [
        'pagespeed_bin',
        '<(pagespeed_root)/build/temp_gyp/protobuf_java.gyp:protobuf_java_jar',
        '<(pagespeed_root)/pagespeed/proto/proto_gen.gyp:pagespeed_proto_java_jar',
      ],
      'actions': [
        {
          'action_name': 'javac',
          'inputs': [
            'java/com/googlecode/page_speed/Pagespeed.java',
          ],
          'outputs': [
            '<(DEPTH)/out/java/classes/pagespeed/com/googlecode/page_speed/Pagespeed.class',
          ],
	  # Assumes javac is in the path.
          'action': [
            'javac',
            '-d', '<(DEPTH)/out/java/classes/pagespeed',
            '-classpath', '<(DEPTH)/out/java/protobuf.jar:<(DEPTH)/out/java/pagespeed_proto.jar',
            'java/com/googlecode/page_speed/Pagespeed.java',
          ],
        },
      ],
    },
  ],
}
