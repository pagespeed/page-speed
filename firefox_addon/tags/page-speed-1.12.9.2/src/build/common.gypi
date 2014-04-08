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
#
# PageSpeed Firefox extension gyp configuration.
{
  'includes': [
    # Import Chromium's common.gypi to inherit their build
    # configuration.
    '../third_party/chromium/src/build/common.gypi',
    # Import pagespeed's override gypi to inherit their overrides.
    '../third_party/libpagespeed/src/build/pagespeed_overrides.gypi',
    # Import our override gypi to modify the Chromium configuration as
    # needed.
    'psff_overrides.gypi',
  ],
}
