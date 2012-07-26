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
  'targets': [
    {
      'target_name': 'All',
      'type': 'none',
      'xcode_create_dependents_test_runner': 1,
      'dependencies': [
        '../base/base.gyp:*',
        '../build/temp_gyp/googleurl.gyp:*',
        '../build/temp_gyp/protobuf_java.gyp:*',
        '../pagespeed/apps/apps.gyp:*',
        '../pagespeed/core/core.gyp:*',
        '../pagespeed/css/css.gyp:*',
        '../pagespeed/filters/filters.gyp:*',
        '../pagespeed/formatters/formatters.gyp:*',
        '../pagespeed/har/har.gyp:*',
        '../pagespeed/html/html.gyp:*',
        '../pagespeed/image_compression/image_compression.gyp:*',
        '../pagespeed/l10n/l10n.gyp:*',
        '../pagespeed/pagespeed.gyp:*',
        '../pagespeed/pdf/pdf.gyp:*',
        '../pagespeed/platform/platform.gyp:*',
        '../pagespeed/proto/proto.gyp:*',
        '../pagespeed/util/util.gyp:*',
      ],
    }
  ]
}
