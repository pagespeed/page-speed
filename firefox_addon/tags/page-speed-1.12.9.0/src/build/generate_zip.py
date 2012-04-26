#!/usr/bin/python

# Copyright 2011 Google Inc.
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

# Zips all files in a directory. This is a python script (as opposed
# to a one-line invocation of the 'zip' binary) so it can cleanly
# integrate with the GYP build system.

# Example:
# ./generate_zip.py output.zip /dir/to/zip

import os
import sys
import zipfile

def ZipDirectory(zip_file, cur_dir):
  for f in os.listdir(cur_dir):
    f = os.path.join(cur_dir, f)
    if os.path.isdir(f):
      ZipDirectory(zip_file, f)
    else:
      zip_file.write(f)

def main(argv):
  output_file = argv[1]
  input_dir = argv[2]

  if os.path.exists(output_file):
    os.remove(output_file)
  zip_file = zipfile.ZipFile(output_file, 'w', zipfile.ZIP_DEFLATED)
  os.chdir(input_dir)
  ZipDirectory(zip_file, '.')
  zip_file.close()

if '__main__' == __name__:
  sys.exit(main(sys.argv))
