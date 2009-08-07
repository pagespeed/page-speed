#!/usr/bin/python2.5
#
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

"""Set up environment for unit tests.  Import app engine paths and mock them."""

import os
import sys

# Import mox, and give directions to install it on failure.
# Mox is not used in this file, but if the import fails
# than the user will get a useful error message.
# Individual tests still need to import mox, because importing
# is not transitive.  Import this file first to give a good
# error message if mox is not installed.
try:
  import mox
except ImportError:
  print ('Test %s requires mox, a python mocking library.  '
         'Please download it from '
         'http://code.google.com/p/pymox/ .') % __file__
  raise

# Need to change the import paths to find classes from the app engine SDK.
APPENGINE_PATH = os.environ["APP_ENGINE_PATH"]

if not APPENGINE_PATH:
  raise ImportError('Please set enviornment variable APP_ENGINE_PATH '
                    'to the root directory of the app engine SDK.  '
                    'The app engine sdk can be downloaded at '
                    'http://code.google.com/appengine/downloads.html .')

import_paths = [
    APPENGINE_PATH,
    os.path.join(APPENGINE_PATH, 'lib', 'django'),
    os.path.join(APPENGINE_PATH, 'lib', 'webob'),
    os.path.join(APPENGINE_PATH, 'lib', 'yaml', 'lib')
]

for path in import_paths:
  if not os.path.exists(path):
    raise ImportError('App engine SDK path %s does not exist' % path)
sys.path = import_paths + sys.path
