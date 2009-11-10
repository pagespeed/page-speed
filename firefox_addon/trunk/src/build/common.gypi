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
  'includes': [
    '../third_party/chromium/src/build/common.gypi',
  ],
  # As of r30253, Chromium's src/build/common.gypi turns on -fvisibility=hidden
  # under certain conditions.  However, that breaks our build for some reason,
  # so the setting below turns it back off.  A better fix for the future might
  # be to add visibility pragmas to our code, or something.  (mdsteele)
  'target_defaults': {
    'cflags!': [
      '-fvisibility=hidden',
    ],
  },
}
